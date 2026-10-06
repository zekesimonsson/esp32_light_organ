#ifndef DMX_OUTPUT_H
#define DMX_OUTPUT_H

#include <stddef.h>
#include <stdint.h>

#define DMX_MAX_CHANNELS 512

void dmx_output_init(void);

void dmx_output_set_channel(
    uint16_t channel,
    uint8_t value);

void dmx_output_send(void);

#endif