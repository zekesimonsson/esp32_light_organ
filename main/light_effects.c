#include "light_effects.h"

#include <string.h>

static float beat_flash = 0.0f;
static int beat_light = -1;
static uint8_t master_brightness = 100;
static uint8_t beat_brightness = 100;
static int alternate_group = 0;

static light_effect_mode_t effect_mode =
    LIGHT_EFFECT_BEAT_CHASE;

static uint8_t apply_brightness(uint8_t value)
{
    return (uint8_t)(
        ((uint16_t)value * master_brightness) / 100);
}

static uint8_t apply_beat_brightness(uint8_t value)
{
    return (uint8_t)(
        ((uint16_t)value * beat_brightness) / 100);
}

static uint8_t level_to_dmx(float level)
{
    if (level < 0.0f)
        level = 0.0f;

    if (level > 1.0f)
        level = 1.0f;

    return (uint8_t)(level * 255.0f);
}

static void effect_classic(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t white,
    light_state_t *state)
{
    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        state->light[i].red =
            apply_brightness(red);

        state->light[i].green =
            apply_brightness(green);

        state->light[i].blue =
            apply_brightness(blue);

        state->light[i].white =
            apply_brightness(white);
    }
}

static void effect_beat_chase(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t white,
    light_state_t *state)
{
    float beat_level = white / 255.0f;

    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        /*
         * Normal RGB is suppressed during the beat.
         * At the start of a beat it is completely off.
         */
        float rgb_factor = 1.0f - (beat_level * 0.65f);

        state->light[i].red =
            apply_brightness(
                (uint8_t)(red * rgb_factor));

        state->light[i].green =
            apply_brightness(
                (uint8_t)(green * rgb_factor));

        state->light[i].blue =
            apply_brightness(
                (uint8_t)(blue * rgb_factor));

        /*
         * Only the selected chase light gets
         * the white beat flash.
         */
        if (i == beat_light)
        {
            state->light[i].white =
                apply_beat_brightness(white);
        }
        else
        {
            state->light[i].white = 0;
        }
    }
}

static void effect_alternate(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t white,
    light_state_t *state)
{
    float beat_level = white / 255.0f;
    float rgb_factor = 1.0f - beat_level;

    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        state->light[i].red =
            apply_brightness(
                (uint8_t)(red * rgb_factor));

        state->light[i].green =
            apply_brightness(
                (uint8_t)(green * rgb_factor));

        state->light[i].blue =
            apply_brightness(
                (uint8_t)(blue * rgb_factor));

        /*
         * Group 0: L1 + L3
         * Group 1: L2 + L4
         */
        if ((i % 2) == alternate_group)
        {
            state->light[i].white =
                apply_beat_brightness(white);
        }
        else
        {
            state->light[i].white = 0;
        }
    }
}

void light_effects_init(void)
{
    beat_flash = 0.0f;
    beat_light = -1;
    alternate_group = 0;
}

void light_effects_process(
    const audio_analysis_t *audio,
    light_state_t *state)
{
    memset(state, 0, sizeof(*state));

    if (audio->beat)
    {
        beat_flash = 1.0f;

        beat_light++;

        if (beat_light >= LIGHT_COUNT)
        {
            beat_light = 0;
        }
        alternate_group = 1 - alternate_group;
    }
    else
    {
        beat_flash *= 0.82f;

        if (beat_flash < 0.02f)
        {
            beat_flash = 0.0f;
        }
    }

    float red_level =
        0.25f + (audio->bass_level * 0.75f);

    uint8_t red =
        level_to_dmx(red_level);

    uint8_t green =
        level_to_dmx(audio->mid_level);

    uint8_t blue =
        level_to_dmx(audio->treble_level);

    uint8_t white =
        level_to_dmx(beat_flash);

    switch (effect_mode)
    {
        case LIGHT_EFFECT_CLASSIC:
            effect_classic(
                red,
                green,
                blue,
                white,
                state);
            break;

        case LIGHT_EFFECT_BEAT_CHASE:
            effect_beat_chase(
                red,
                green,
                blue,
                white,
                state);
            break;

        case LIGHT_EFFECT_ALTERNATE:
            effect_alternate(
                red,
                green,
                blue,
                white,
                state);
            break;

        default:
            break;
    }
}

void light_effects_set_brightness(uint8_t percent)
{
    if (percent > 100)
    {
        percent = 100;
    }

    master_brightness = percent;
}

void light_effects_set_beat_brightness(uint8_t percent)
{
    if (percent > 100)
    {
        percent = 100;
    }

    beat_brightness = percent;
}


void light_effects_set_mode(
    light_effect_mode_t mode)
{
    if (mode < LIGHT_EFFECT_COUNT)
    {
        effect_mode = mode;
    }
}

