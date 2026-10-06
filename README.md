# ESP32 Light Organ

A music-reactive lighting controller based on the ESP32-S3.

The controller captures audio using an I2S MEMS microphone, analyzes the
audio in real time and generates RGBW lighting effects. The final system
will control four RGBW PAR fixtures using DMX512.

## System overview

```text
I2S Microphone
      |
      v
+-------------+
| audio_input |
+-------------+
      |
      | Audio samples
      v
+----------------+
| audio_analyzer |
+----------------+
      |
      | Frequency levels + beat
      v
+---------------+
| light_effects |
+---------------+
      |
      | RGBW values for 4 lights
      v
+------------+
| dmx_output |     (planned)
+------------+
      |
      | DMX512
      v
4 x RGBW PAR fixtures
```

## Hardware

- ESP32-S3-DevKitC-1-N8R8
- SPH0645 I2S MEMS microphone
- Isolated RS485/DMX interface
- 4 x RGBW DMX PAR fixtures

## Audio analysis

Audio is sampled at 48 kHz.

The analyzer uses a 1024-point FFT and divides the audio spectrum into
five frequency bands:

| Band | Frequency |
|---|---:|
| Bass | 40–200 Hz |
| Low-mid | 200–500 Hz |
| Mid | 500–2000 Hz |
| High-mid | 2000–6000 Hz |
| Treble | 6000–16000 Hz |

Each frequency band is automatically normalized to a value between
0.0 and 1.0.

The analyzer also detects bass transients which are used as beat events.

## Lighting effects

The current lighting effect maps:

- Bass -> Red
- Mid -> Green
- Treble -> Blue
- Beat -> White flash

The white beat flash moves between the four fixtures:

```text
Beat 1:  X . . .
Beat 2:  . X . .
Beat 3:  . . X .
Beat 4:  . . . X
Beat 5:  X . . .
```

The beat flash also has a short decay to create a smoother visual effect.

## Software modules

### audio_input

Configures the ESP32-S3 I2S peripheral and reads samples from the
SPH0645 microphone.

### audio_analyzer

Performs:

- DC removal
- RMS calculation
- Hann window
- FFT
- Frequency-band analysis
- Automatic level normalization
- Beat detection

### light_effects

Converts the analyzed audio into RGBW values for four independent
lighting fixtures.

### dmx_output

Planned module responsible for converting the RGBW light states into
DMX512 channels and transmitting them over an isolated RS485 interface.

## Development environment

- ESP32-S3
- ESP-IDF 6.1
- ESP-DSP
- Visual Studio Code
- Windows

## Status

Currently implemented:

- [x] I2S microphone input
- [x] RMS audio measurement
- [x] 1024-point FFT
- [x] Frequency-band analysis
- [x] Automatic audio level normalization
- [x] Beat detection
- [x] RGBW light-effect generation
- [x] Four-light beat chase
- [ ] DMX512 output
- [ ] DMX fixture mapping
- [ ] Hardware integration
