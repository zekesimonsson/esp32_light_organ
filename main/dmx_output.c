#include "dmx_output.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

#define DMX_UART       UART_NUM_1
#define DMX_TX_GPIO    GPIO_NUM_17
#define DMX_RX_GPIO    GPIO_NUM_18

#define DMX_BAUD_RATE  250000
#define DMX_DEFAULT_CHANNELS  32

static uint16_t dmx_channel_count = DMX_DEFAULT_CHANNELS;
/*
 * Byte 0 = DMX start code
 * Byte 1 = DMX channel 1
 * ...
 * Byte 512 = DMX channel 512
 */
static uint8_t dmx_data[DMX_MAX_CHANNELS + 1];

void dmx_output_init(void)
{
    memset(dmx_data, 0, sizeof(dmx_data));

    uart_config_t uart_config = {
        .baud_rate = DMX_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(
        uart_driver_install(
            DMX_UART,
            1024,
            0,
            0,
            NULL,
            0));

    ESP_ERROR_CHECK(
        uart_param_config(
            DMX_UART,
            &uart_config));

    ESP_ERROR_CHECK(
        uart_set_pin(
            DMX_UART,
            DMX_TX_GPIO,
            DMX_RX_GPIO,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE));

    /*
     * DMX start code = 0
     */
    dmx_data[0] = 0;
}

void dmx_output_set_channel(
    uint16_t channel,
    uint8_t value)
{
    if ((channel < 1) ||
        (channel > DMX_MAX_CHANNELS))
    {
        return;
    }

    dmx_data[channel] = value;
}

void dmx_output_send(void)
{
    /*
     * Make sure previous transmission is complete.
     */
    ESP_ERROR_CHECK(
        uart_wait_tx_done(
            DMX_UART,
            pdMS_TO_TICKS(100)));

    /*
     * DMX BREAK.
     *
     * Temporarily drive TX low.
     */
    ESP_ERROR_CHECK(
        uart_set_line_inverse(
            DMX_UART,
            UART_SIGNAL_TXD_INV));

    esp_rom_delay_us(120);

    /*
     * Return TX to normal state.
     */
    ESP_ERROR_CHECK(
        uart_set_line_inverse(
            DMX_UART,
            0));

    /*
     * Mark After Break.
     */
    esp_rom_delay_us(12);

    /*
     * Start code + 512 channels.
     */
    uart_write_bytes(
        DMX_UART,
        dmx_data,
        dmx_channel_count + 1);
}