#include "light_effects.h"

#include <string.h>

static float beat_flash = 0.0f;
static int beat_light = -1;

static uint8_t level_to_dmx(float level)
{
    if (level < 0.0f)
        level = 0.0f;

    if (level > 1.0f)
        level = 1.0f;

    return (uint8_t)(level * 255.0f);
}

void light_effects_init(void)
{
    beat_flash = 0.0f;
    beat_light = -1;
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
    }
    else
    {
        beat_flash *= 0.72f;

        if (beat_flash < 0.02f)
        {
            beat_flash = 0.0f;
        }
    }

    uint8_t red =
        level_to_dmx(audio->bass_level);

    uint8_t green =
        level_to_dmx(audio->mid_level);

    uint8_t blue =
        level_to_dmx(audio->treble_level);

    uint8_t white =
        level_to_dmx(beat_flash);

    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        state->light[i].red   = red;
        state->light[i].green = green;
        state->light[i].blue  = blue;

        if (i == beat_light)
        {
            state->light[i].white = white;
        }
        else
        {
            state->light[i].white = 0;
        }
    }
}