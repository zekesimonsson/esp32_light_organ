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
            "RMS:%8.0f  "
            "B:%10.0f  "
            "LM:%10.0f  "
            "M:%10.0f  "
            "HM:%10.0f  "
            "T:%10.0f\n",
            analysis.rms,
            analysis.bass,
            analysis.low_mid,
            analysis.mid,
            analysis.high_mid,
            analysis.treble
        );
    }
}}
