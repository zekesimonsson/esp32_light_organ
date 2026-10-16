#include <stdint.h>
#include <string.h>
#include <math.h>

#include "esp_timer.h"
#include "light_effects.h"

#define AUTO_INTERVAL_US (30LL * 1000000LL)

static float beat_flash = 0.0f;
static int beat_light = -1;
static uint8_t master_brightness = 100;
static uint8_t beat_brightness = 100;
static int alternate_group = 0;
static light_effect_mode_t auto_effect = LIGHT_EFFECT_CLASSIC;
static int64_t auto_last_switch_us = 0;
static int color_chase_position = 0;

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
    static const uint8_t colors[4][3] = {
        {255,   0,   0},
        {  0, 255,   0},
        {  0,   0, 255},
        {180,   0, 255}
    };

    float music_level =
        (red + green + blue) / (3.0f * 255.0f);

    float background =
        (0.20f + 0.35f * music_level) *
        (float)master_brightness / 100.0f;

    float beat_level = white / 255.0f;

    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        int color_index =
            (i - color_chase_position + LIGHT_COUNT)
            % LIGHT_COUNT;

        float intensity = background;

        if (i == beat_light)
        {
            float pulse =
                beat_level * 0.75f *
                (float)beat_brightness / 100.0f;

            intensity += pulse;
        }

        if (intensity > 1.0f)
        {
            intensity = 1.0f;
        }

        state->light[i].red =
            (uint8_t)(colors[color_index][0] * intensity);

        state->light[i].green =
            (uint8_t)(colors[color_index][1] * intensity);

        state->light[i].blue =
            (uint8_t)(colors[color_index][2] * intensity);

        state->light[i].white = 0;
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

static void effect_color_chase(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t white,
    light_state_t *state)
{
    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        int position =
            (i - color_chase_position + LIGHT_COUNT)
            % LIGHT_COUNT;

        switch (position)
        {
            case 0:
                // Bass-driven red
                state->light[i].red =
                    apply_brightness(red);
                break;

            case 1:
                // Mid-driven green
                state->light[i].green =
                    apply_brightness(green);
                break;

            case 2:
                // Treble-driven blue
                state->light[i].blue =
                    apply_brightness(blue);
                break;

            case 3:
                // White flash on beat
                state->light[i].white =
                    apply_beat_brightness(white);
                break;

            default:
                break;
        }
    }
}

#define COLOR_WAVE_PERIOD_US (8LL * 1000000LL)
#define TWO_PI 6.28318530718f

static void effect_color_wave(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t white,
    light_state_t *state)
{
    // Pure RGB colors: red, green, blue, purple
    static const uint8_t colors[4][3] = {
        {255,   0,   0},
        {  0, 255,   0},
        {  0,   0, 255},
        {180,   0, 255}
    };

    int64_t now = esp_timer_get_time();

    // Position advances continuously through four colors
    float position =
        4.0f *
        (float)(now % COLOR_WAVE_PERIOD_US) /
        (float)COLOR_WAVE_PERIOD_US;

    // Music intensity with a minimum background level
    float music_level =
        (red + green + blue) / (3.0f * 255.0f);

    float intensity = 0.30f + 0.70f * music_level;

    // Extra brightness pulse on beat
    float beat_level = white / 255.0f;
    intensity += beat_level * 0.35f;

    if (intensity > 1.0f)
    {
        intensity = 1.0f;
    }

    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        float p = position + (float)i;

        int color_index = ((int)p) % 4;
        int next_index = (color_index + 1) % 4;

        float blend = p - (float)((int)p);

        // Smooth transition between colors
        float r =
            colors[color_index][0] * (1.0f - blend) +
            colors[next_index][0] * blend;

        float g =
            colors[color_index][1] * (1.0f - blend) +
            colors[next_index][1] * blend;

        float b =
            colors[color_index][2] * (1.0f - blend) +
            colors[next_index][2] * blend;

        state->light[i].red =
            apply_brightness((uint8_t)(r * intensity));

        state->light[i].green =
            apply_brightness((uint8_t)(g * intensity));

        state->light[i].blue =
            apply_brightness((uint8_t)(b * intensity));

        // No white channel
        state->light[i].white = 0;
    }
}


#define WHITE_FLASH_DURATION_US 50000LL  // 50 ms

static int64_t white_flash_start_us = 0;

static void effect_color_chase_flash(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    uint8_t white,
    light_state_t *state)
{
    // Start with the existing Color Chase effect
    effect_color_chase(red, green, blue, white, state);

    int64_t now = esp_timer_get_time();

    // Check whether the white flash is active
    if (white_flash_start_us != 0 &&
        now - white_flash_start_us < WHITE_FLASH_DURATION_US)
    {
        // Flash all four lamps
        for (int i = 0; i < LIGHT_COUNT; i++)
        {
            state->light[i].white =
                apply_beat_brightness(255);
        }
    }
    else
    {
        // No white light between beats
        for (int i = 0; i < LIGHT_COUNT; i++)
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
    color_chase_position = 0;
}

void light_effects_process(
    const audio_analysis_t *audio,
    light_state_t *state)
{
    light_effect_mode_t active_mode = effect_mode;

    if (effect_mode == LIGHT_EFFECT_AUTO)
    {   
        int64_t now = esp_timer_get_time();

        if (now - auto_last_switch_us >= AUTO_INTERVAL_US)
        {
            switch (auto_effect)
            {
                case LIGHT_EFFECT_CLASSIC:
                    auto_effect = LIGHT_EFFECT_BEAT_CHASE;
                    break;

                case LIGHT_EFFECT_BEAT_CHASE:
                    auto_effect = LIGHT_EFFECT_ALTERNATE;
                    break;

                case LIGHT_EFFECT_ALTERNATE:
                    auto_effect = LIGHT_EFFECT_COLOR_CHASE;
                    break;

                case LIGHT_EFFECT_COLOR_CHASE:
                    auto_effect = LIGHT_EFFECT_COLOR_WAVE;
                    break;
                case LIGHT_EFFECT_COLOR_WAVE:
                    auto_effect = LIGHT_EFFECT_COLOR_CHASE_FLASH;
                    break;

                case LIGHT_EFFECT_COLOR_CHASE_FLASH:
                    auto_effect = LIGHT_EFFECT_CLASSIC;
                    break;
 
                default:
                    auto_effect = LIGHT_EFFECT_CLASSIC;
                    break;
            }

            auto_last_switch_us = now;
        }

        active_mode = auto_effect;
    }    

    memset(state, 0, sizeof(*state));

    if (audio->beat)
    {
        beat_flash = 1.0f;
        white_flash_start_us = esp_timer_get_time();

        color_chase_position =
            (color_chase_position + 1) % LIGHT_COUNT;

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

    switch (active_mode)
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

        case LIGHT_EFFECT_COLOR_CHASE:
            effect_color_chase(
                red,
                green,
                blue,
                white,
                state);
            break;

        case LIGHT_EFFECT_COLOR_WAVE:
            effect_color_wave(
                red,
                green,
                blue,
                white,
                state);
            break;

        case LIGHT_EFFECT_COLOR_CHASE_FLASH:
            effect_color_chase_flash(
                red, green, blue, white, state);
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


void light_effects_set_mode(light_effect_mode_t mode)
{
    if (mode >= LIGHT_EFFECT_COUNT)
    {
        return;
    }

    if (mode == LIGHT_EFFECT_AUTO &&
        effect_mode != LIGHT_EFFECT_AUTO)
    {
        auto_effect = LIGHT_EFFECT_CLASSIC;
        auto_last_switch_us = esp_timer_get_time();
    }

    effect_mode = mode;
}

