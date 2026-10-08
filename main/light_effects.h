#ifndef LIGHT_EFFECTS_H
#define LIGHT_EFFECTS_H

#include <stdint.h>
#include "audio_analyzer.h"

#define LIGHT_COUNT 4

typedef struct
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white;
} light_t;

typedef struct
{
    light_t light[LIGHT_COUNT];
} light_state_t;

typedef enum
{
    LIGHT_EFFECT_CLASSIC = 0,
    LIGHT_EFFECT_BEAT_CHASE,
    LIGHT_EFFECT_COUNT
} light_effect_mode_t;

void light_effects_init(void);

void light_effects_process(
    const audio_analysis_t *audio,
    light_state_t *state);

void light_effects_set_brightness(uint8_t percent);
void light_effects_set_beat_brightness(uint8_t percent);
void light_effects_set_mode(light_effect_mode_t mode);

#endif