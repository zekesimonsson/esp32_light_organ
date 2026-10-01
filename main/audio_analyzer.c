#include "audio_analyzer.h"
#include "audio_input.h"

#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_err.h"

#include "esp_dsp.h"


#define FFT_SIZE 1024

static float fft_data[FFT_SIZE * 2];

static int32_t fft_samples[FFT_SIZE];
static size_t fft_sample_count = 0;

static float bass_average = 0.0f;
static bool beat_initialized = false;
static int beat_holdoff = 0;

typedef struct
{
    float noise_floor;
    float peak;
    bool initialized;

} band_normalizer_t;

static band_normalizer_t bass_normalizer;
static band_normalizer_t low_mid_normalizer;
static band_normalizer_t mid_normalizer;
static band_normalizer_t high_mid_normalizer;
static band_normalizer_t treble_normalizer;

void audio_analyzer_init(void)
{
    esp_err_t ret =
        dsps_fft2r_init_fc32(
            NULL,
            FFT_SIZE
        );

    if (ret != ESP_OK)
    {
        printf(
            "FFT init error: %d\n",
            ret
        );
    }
}


static float calculate_band(
    float min_frequency,
    float max_frequency)
{
    const float bin_width =
        (float)AUDIO_SAMPLE_RATE / FFT_SIZE;

    int first_bin =
        (int)ceilf(min_frequency / bin_width);

    int last_bin =
        (int)floorf(max_frequency / bin_width);

    if (first_bin < 1)
    {
        first_bin = 1;
    }

    if (last_bin >= FFT_SIZE / 2)
    {
        last_bin = (FFT_SIZE / 2) - 1;
    }

    float sum = 0.0f;
    int bin_count = 0;

    for (int bin = first_bin;
         bin <= last_bin;
         bin++)
    {
        float real = fft_data[bin * 2];
        float imag = fft_data[bin * 2 + 1];

        float magnitude =
            sqrtf(real * real + imag * imag);

        sum += magnitude;
        bin_count++;
    }

    if (bin_count == 0)
    {
        return 0.0f;
    }

    return sum / bin_count;
}

static float normalize_band(
    float value,
    band_normalizer_t *normalizer)
{
    /*
     * Initialize from first measurement.
     */
    if (!normalizer->initialized)
    {
        normalizer->noise_floor = value;
        normalizer->peak = value * 2.0f;
        normalizer->initialized = true;

        return 0.0f;
    }

    /*
     * Noise floor follows downward changes relatively
     * quickly, but upward changes very slowly.
     *
     * This allows the analyzer to adapt to the
     * background noise of the room.
     */
    if (value < normalizer->noise_floor)
    {
        normalizer->noise_floor =
            normalizer->noise_floor * 0.90f +
            value * 0.10f;
    }
    else
    {
        normalizer->noise_floor =
            normalizer->noise_floor * 0.999f +
            value * 0.001f;
    }

    /*
     * Peak follows new peaks immediately.
     * Otherwise it slowly decays.
     */
    if (value > normalizer->peak)
    {
        normalizer->peak = value;
    }
    else
    {
        normalizer->peak *= 0.995f;
    }

    /*
     * Peak must always stay sufficiently above
     * the noise floor.
     */
    float minimum_range =
        normalizer->noise_floor * 0.5f;

    if (minimum_range < 1.0f)
    {
        minimum_range = 1.0f;
    }

    if (normalizer->peak <
        normalizer->noise_floor + minimum_range)
    {
        normalizer->peak =
            normalizer->noise_floor + minimum_range;
    }

    /*
     * Normalize to 0.0 - 1.0.
     */
    float level =
        (value - normalizer->noise_floor) /
        (normalizer->peak -
         normalizer->noise_floor);

    if (level < 0.0f)
    {
        level = 0.0f;
    }

    if (level > 1.0f)
    {
        level = 1.0f;
    }

    /*
     * Small dead zone to prevent lights flickering
     * due to background noise.
     */
    const float dead_zone = 0.05f;

    if (level < dead_zone)
    {
        level = 0.0f;
    }
    else
    {
        level =
            (level - dead_zone) /
            (1.0f - dead_zone);
    }

    return level;
}

static bool detect_beat(float bass_level)
{
    if (!beat_initialized)
    {
        bass_average = bass_level;
        beat_initialized = true;
        return false;
    }

    /*
     * Slowly moving reference level.
     */
    bass_average =
        0.95f * bass_average +
        0.05f * bass_level;

    /*
     * Prevent multiple detections of the same beat.
     */
    if (beat_holdoff > 0)
    {
        beat_holdoff--;
        return false;
    }

    /*
     * A beat must:
     *
     * 1. Have a reasonably strong bass level.
     * 2. Be significantly stronger than
     *    the recent average.
     */
    if ((bass_level > 0.55f) &&
        (bass_level > bass_average + 0.25f))
    {
        beat_holdoff = 5;
        return true;
    }

    return false;
}

bool audio_analyzer_process(
    const int32_t *samples,
    size_t sample_count,
    audio_analysis_t *result)
{
    /*
     * Collect samples until we have enough
     * for one complete FFT.
     */
    for (size_t i = 0;
         (i < sample_count) &&
         (fft_sample_count < FFT_SIZE);
         i++)
    {
        fft_samples[fft_sample_count] =
            samples[i];

        fft_sample_count++;
    }

    /*
     * Not enough samples yet.
     */
    if (fft_sample_count < FFT_SIZE)
    {
        return false;
    }

    memset(result, 0, sizeof(*result));

    /*
     * Calculate DC component.
     */
    double sum = 0.0;

    for (int i = 0; i < FFT_SIZE; i++)
    {
        sum += fft_samples[i];
    }

    double mean = sum / FFT_SIZE;


    /*
     * Remove DC and calculate RMS.
     *
     * At the same time prepare the complex FFT input.
     */
    double sum_squared = 0.0;

    for (int i = 0; i < FFT_SIZE; i++)
    {
        float sample =
            (float)((double)fft_samples[i] - mean);

        sum_squared +=
            (double)sample * sample;

        /*
         * Hann window.
         */
        float window =
            0.5f -
            0.5f *
            cosf(
                2.0f *
                (float)M_PI *
                i /
                (FFT_SIZE - 1)
            );

        fft_data[i * 2] =
            sample * window;

        fft_data[i * 2 + 1] = 0.0f;
    }

    result->rms =
        sqrtf(
            (float)(
                sum_squared / FFT_SIZE
            )
        );


    /*
     * FFT
     */
    esp_err_t ret;

    ret = dsps_fft2r_fc32_ansi(
        fft_data,
        FFT_SIZE
    );

    if (ret != ESP_OK)
    {
        printf("FFT error: %d\n", ret);
        fft_sample_count = 0;
        return false;
    }

    ret = dsps_bit_rev_fc32_ansi(
        fft_data,
        FFT_SIZE
    );

    if (ret != ESP_OK)
    {
        printf("Bit reverse error: %d\n", ret);
        fft_sample_count = 0;
        return false;
    }


    /*
     * Frequency bands.
     */
    result->bass =
        calculate_band(40.0f, 200.0f);

    result->low_mid =
        calculate_band(200.0f, 500.0f);

    result->mid =
        calculate_band(500.0f, 2000.0f);

    result->high_mid =
        calculate_band(2000.0f, 6000.0f);

    result->treble =
        calculate_band(6000.0f, 16000.0f);

    /*
     * Normalize frequency bands.
     */
    result->bass_level =
        normalize_band(
            result->bass,
            &bass_normalizer
        );

    result->low_mid_level =
        normalize_band(
            result->low_mid,
            &low_mid_normalizer
        );

    result->mid_level =
        normalize_band(
            result->mid,
            &mid_normalizer
        );

    result->high_mid_level =
        normalize_band(
            result->high_mid,
            &high_mid_normalizer
        );

    result->treble_level =
        normalize_band(
            result->treble,
            &treble_normalizer
        );
    
    result->beat =
    detect_beat(result->bass_level);
    /*
     * Start collecting the next FFT block.
     */
    fft_sample_count = 0;

    return true;
}
