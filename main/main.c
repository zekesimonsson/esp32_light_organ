#include <stdio.h>
#include <stdint.h>

#include "audio_input.h"
#include "audio_analyzer.h"
#include "light_effects.h"
#include "dmx_output.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void dmx_task(void *arg)
{
    while (1)
    {
        dmx_output_send();

        /*
         * Approximately 30 DMX frames per second.
         */
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}

void app_main(void)
{
    printf("\n");
    printf("=============================\n");
    printf(" LIGHT ORGAN - FFT TEST\n");
    printf("=============================\n\n");

    audio_input_init();
    audio_analyzer_init();
    light_effects_init();
    light_effects_set_brightness(30);
    light_effects_set_mode(LIGHT_EFFECT_CLASSIC);
    dmx_output_init();

    xTaskCreate(
        dmx_task,
        "dmx_task",
        4096,
        NULL,
        5,
        NULL);

    int32_t samples[AUDIO_BLOCK_SIZE];

    while (1)
    {
        size_t sample_count =
            audio_input_read(
                samples,
                AUDIO_BLOCK_SIZE
            );

        audio_analysis_t analysis;

        if (audio_analyzer_process(samples, sample_count, &analysis))
        {
            light_state_t lights;

            light_effects_process(
                &analysis,
                &lights);

            for (int i = 0; i < LIGHT_COUNT; i++)
            {
                uint16_t base_channel = 1 + (i * 4);

                dmx_output_set_channel(
                    base_channel + 0,
                    lights.light[i].red);

                dmx_output_set_channel(
                    base_channel + 1,
                    lights.light[i].green);

                dmx_output_set_channel(
                    base_channel + 2,
                    lights.light[i].blue);

                dmx_output_set_channel(
                    base_channel + 3,
                    lights.light[i].white);
            }

            printf(
                "%s | "
                "L1:%3d %3d %3d %3d | "
                "L2:%3d %3d %3d %3d | "
                "L3:%3d %3d %3d %3d | "
                "L4:%3d %3d %3d %3d\n",

                analysis.beat ? "BEAT" : "    ",

                lights.light[0].red,
                lights.light[0].green,
                lights.light[0].blue,
                lights.light[0].white,

                lights.light[1].red,
                lights.light[1].green,
                lights.light[1].blue,
                lights.light[1].white,

                lights.light[2].red,
                lights.light[2].green,
                lights.light[2].blue,
                lights.light[2].white,

                lights.light[3].red,
                lights.light[3].green,
                lights.light[3].blue,
                lights.light[3].white
            );
        }
    }
}