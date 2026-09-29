#ifndef AUDIO_INPUT_H
#define AUDIO_INPUT_H

#include <stddef.h>
#include <stdint.h>

#define AUDIO_SAMPLE_RATE  48000
#define AUDIO_BLOCK_SIZE   512

/**
 * Initialize I2S and the SPH0645 microphone.
 */
void audio_input_init(void);

/**
 * Read one block of audio samples.
 *
 * The SPH0645 is configured on the left I2S channel.
 * The function removes the unused I2S channel and returns
 * only valid microphone samples.
 *
 * @param samples       Destination buffer
 * @param max_samples   Size of destination buffer
 *
 * @return Number of valid samples written to buffer
 */
size_t audio_input_read(int32_t *samples, size_t max_samples);

#endif