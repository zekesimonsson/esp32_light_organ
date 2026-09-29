#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "audio_input.h"


static double calculate_rms(
    const int32_t *samples,
    size_t sample_count)
{
    if (sample_count == 0)
    {
        return 0.0;
    }

    /*
     * Calculate DC component.
     */
    int64_t sum = 0;

    for (size_t i = 0; i < sample_count; i++)
    {
        sum += samples[i];
    }

    double mean =
        (double)sum / sample_count;

    /*
     * Calculate RMS after removing DC.
     */
    double sum_squared = 0.0;

    for (size_t i = 0; i < sample_count; i++)
    {
        double sample =
            (double)samples[i] - mean;

        sum_squared += sample * sample;
    }

    return sqrt(
        sum_squared / sample_count
    );
}


void app_main(void)
{
    printf("\n");
    printf("=============================\n");
    printf(" LIGHT ORGAN\n");
    printf("=============================\n\n");

    audio_input_init();

    int32_t samples[AUDIO_BLOCK_SIZE];

    while (1)
    {
        size_t sample_count =
            audio_input_read(
                samples,
                AUDIO_BLOCK_SIZE
            );

        double rms =
            calculate_rms(
                samples,
                sample_count
            );

        printf("RMS: %9.0f\n", rms);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}