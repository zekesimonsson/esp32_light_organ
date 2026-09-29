#ifndef AUDIO_ANALYZER_H
#define AUDIO_ANALYZER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    float rms;

    float bass;
    float low_mid;
    float mid;
    float high_mid;
    float treble;

} audio_analysis_t;


/**
 * Initialize the audio analyzer.
 */
void audio_analyzer_init(void);


/**
 * Analyze one block of audio samples.
 */
bool audio_analyzer_process(
    const int32_t *samples,
    size_t sample_count,
    audio_analysis_t *result);

#endif