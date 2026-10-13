#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "light_effects.h"

typedef enum
{
    LIGHT_CONTROL_SET_MODE,
    LIGHT_CONTROL_SET_BRIGHTNESS,
    LIGHT_CONTROL_SET_BEAT_BRIGHTNESS,
    LIGHT_CONTROL_SET_BLACKOUT
} light_control_command_type_t;

typedef struct
{
    light_control_command_type_t type;
    uint8_t value;
} light_control_command_t;

typedef struct
{
    light_effect_mode_t mode;
    uint8_t brightness;
    uint8_t beat_brightness;
    bool blackout;
} light_control_status_t;

void light_control_init(void);

bool light_control_send(
    const light_control_command_t *command);

void light_control_process(void);

void light_control_get_status(
    light_control_status_t *status);

#endif