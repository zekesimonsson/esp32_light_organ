#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2s_std.h"
#include "esp_err.h"

#define I2S_BCLK_GPIO   GPIO_NUM_4
#define I2S_DOUT_GPIO   GPIO_NUM_5
#define I2S_LRCLK_GPIO  GPIO_NUM_6

#define SAMPLE_RATE     48000
#define NUM_SAMPLES     512

static i2s_chan_handle_t rx_handle;

static void microphone_init(void)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

    ESP_ERROR_CHECK(
        i2s_new_channel(&chan_cfg, NULL, &rx_handle)
    );

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),

        .slot_cfg =
            I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
                I2S_DATA_BIT_WIDTH_32BIT,
                I2S_SLOT_MODE_MONO
            ),

        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_GPIO,
            .ws   = I2S_LRCLK_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_DOUT_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ESP_ERROR_CHECK(
        i2s_channel_init_std_mode(rx_handle, &std_cfg)
    );

    ESP_ERROR_CHECK(
        i2s_channel_enable(rx_handle)
    );
}


void app_main(void)
{
    printf("\n");
    printf("=============================\n");
    printf(" LIGHT ORGAN - RMS TEST\n");
    printf("=============================\n\n");

    microphone_init();

    int32_t samples[NUM_SAMPLES];
    size_t bytes_read;

    while (1)
    {
        ESP_ERROR_CHECK(
            i2s_channel_read(
                rx_handle,
                samples,
                sizeof(samples),
                &bytes_read,
                portMAX_DELAY
            )
        );

        int sample_count = bytes_read / sizeof(int32_t);

        /*
         * Microphone data is in every second 32-bit word.
         * SEL = GND -> left channel.
         */
        int valid_samples = 0;
        int64_t sum = 0;

        for (int i = 0; i < sample_count; i += 2)
        {
            int32_t sample = samples[i] >> 8;

            sum += sample;
            valid_samples++;
        }

        if (valid_samples == 0)
        {
            continue;
        }

        /*
         * Calculate DC offset.
         */
        double mean = (double)sum / valid_samples;

        /*
         * Calculate RMS after removing DC.
         */
        double sum_squared = 0.0;

        for (int i = 0; i < sample_count; i += 2)
        {
            double sample = (double)(samples[i] >> 8);

            double ac_sample = sample - mean;

            sum_squared += ac_sample * ac_sample;
        }

        double rms = sqrt(sum_squared / valid_samples);

        printf("RMS: %9.0f\n", rms);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
