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
     * Start collecting the next FFT block.
    */
    fft_sample_count = 0;

    return true;
}
