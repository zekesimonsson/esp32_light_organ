#include "audio_input.h"

#include "freertos/FreeRTOS.h"

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"

#define I2S_BCLK_GPIO   GPIO_NUM_4
#define I2S_DOUT_GPIO   GPIO_NUM_5
#define I2S_LRCLK_GPIO  GPIO_NUM_6

static i2s_chan_handle_t rx_handle;


/*
 * The I2S peripheral currently delivers both channel slots.
 * The SPH0645 is connected to the left channel (SEL = GND),
 * so every second 32-bit word contains microphone data.
 *
 * We therefore need a temporary buffer twice the size of the
 * audio buffer returned to the application.
 */
static int32_t i2s_buffer[AUDIO_BLOCK_SIZE * 2];


void audio_input_init(void)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(
            I2S_NUM_0,
            I2S_ROLE_MASTER
        );

    ESP_ERROR_CHECK(
        i2s_new_channel(
            &chan_cfg,
            NULL,
            &rx_handle
        )
    );

    i2s_std_config_t std_cfg = {
        .clk_cfg =
            I2S_STD_CLK_DEFAULT_CONFIG(
                AUDIO_SAMPLE_RATE
            ),

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
        i2s_channel_init_std_mode(
            rx_handle,
            &std_cfg
        )
    );

    ESP_ERROR_CHECK(
        i2s_channel_enable(rx_handle)
    );
}


size_t audio_input_read(
    int32_t *samples,
    size_t max_samples)
{
    if (max_samples > AUDIO_BLOCK_SIZE)
    {
        max_samples = AUDIO_BLOCK_SIZE;
    }

    size_t bytes_read = 0;

    /*
     * Read twice as many I2S words because only every
     * second word contains microphone data.
     */
    ESP_ERROR_CHECK(
        i2s_channel_read(
            rx_handle,
            i2s_buffer,
            max_samples * 2 * sizeof(int32_t),
            &bytes_read,
            portMAX_DELAY
        )
    );

    size_t words_read =
        bytes_read / sizeof(int32_t);

    size_t output_count = 0;

    for (size_t i = 0;
         (i < words_read) && (output_count < max_samples);
         i += 2)
    {
        /*
         * SPH0645 sample is contained in the upper
         * bits of the 32-bit I2S word.
         */
        samples[output_count] =
            i2s_buffer[i] >> 8;

        output_count++;
    }

    return output_count;
}
