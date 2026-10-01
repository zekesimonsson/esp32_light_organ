#include <stdio.h>
#include <stdint.h>

#include "audio_input.h"
#include "audio_analyzer.h"


void app_main(void)
{
    printf("\n");
    printf("=============================\n");
    printf(" LIGHT ORGAN - FFT TEST\n");
    printf("=============================\n\n");

    audio_input_init();
    audio_analyzer_init();

    int32_t samples[AUDIO_BLOCK_SIZE];

while (1)
{
    size_t sample_count =
        audio_input_read(
            samples,
            AUDIO_BLOCK_SIZE
        );

    audio_analysis_t analysis;

    if (audio_analyzer_process(
            samples,
            sample_count,
            &analysis))
    {
        printf(
            "B:%3.0f%% "
            "LM:%3.0f%% "
            "M:%3.0f%% "
            "HM:%3.0f%% "
            "T:%3.0f%% "
            "%s\n",
            analysis.bass_level * 100.0f,
            analysis.low_mid_level * 100.0f,
            analysis.mid_level * 100.0f,
            analysis.high_mid_level * 100.0f,
            analysis.treble_level * 100.0f,
            analysis.beat ? "<<< BEAT >>>" : ""
        );
    }
}}
