#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "light_effects.h"

typedef enum
{
    LIGHT_CONTROL_SET_MODE,
    LIGHT_CONTROL_SET_BRIGHTNESS,
    LIGHT_CONTROL_SET_BEAT_BRIGHTNESS
} light_control_command_type_t;

typedef struct
{
    light_control_command_type_t type;
    uint8_t value;
} light_control_command_t;

void light_control_init(void);

bool light_control_send(
    const light_control_command_t *command);

void light_control_process(void);

#endif