#ifndef DEVICE_NABU_CASA_CONFIG_H
#define DEVICE_NABU_CASA_CONFIG_H

/**
 * Nabu Casa Voice Preview Edition Device Configuration
 *
 * Hardware:
 * - ESP32-S3 (16MB Flash, 8MB PSRAM)
 * - XMOS XU316 Audio Processor (echo cancellation, noise suppression)
 * - TI AIC3204 Audio Codec (I2S)
 * - Dual microphones (far-field)
 * - LED Ring (12 LEDs)
 * - Physical controls (button, rotary dial, mute switch)
 */

// Device identification
#define DEVICE_NAME             "Nabu Casa Voice PE"
#define DEVICE_MODEL            "Voice-Preview-Edition"
#define DEVICE_MANUFACTURER     "Nabu Casa"

// Hardware capabilities
#define DEVICE_HAS_DISPLAY      false   // LED ring only, no screen
#define DEVICE_HAS_TOUCHSCREEN  false
#define DEVICE_HAS_LED_RING     true
#define DEVICE_HAS_PHYSICAL_BUTTONS true
#define DEVICE_HAS_VOLUME_DIAL  true    // Rotary encoder
#define DEVICE_HAS_MUTE_SWITCH  true    // Hardware mute switch

// Audio configuration
#define DEVICE_AUDIO_CODEC      "AIC3204"
#define DEVICE_AUDIO_PROCESSOR  "XMOS-XU316"
#define DEVICE_SAMPLE_RATE      48000   // 48kHz for high quality
#define DEVICE_BITS_PER_SAMPLE  16
#define DEVICE_AUDIO_CHANNELS   2       // Stereo
#define DEVICE_HAS_ECHO_CANCEL  true    // XMOS handles AEC
#define DEVICE_HAS_NOISE_SUPPRESS true  // XMOS handles noise suppression
#define DEVICE_MIC_COUNT        2       // Dual microphones

// LED Ring configuration
#define DEVICE_LED_COUNT        12      // 12 RGB LEDs
#define DEVICE_LED_TYPE         "WS2812B"

// No display
#define DEVICE_DISPLAY_TYPE     "NONE"
#define DEVICE_DISPLAY_WIDTH    0
#define DEVICE_DISPLAY_HEIGHT   0
#define DEVICE_COLOR_DEPTH      0

// I2S configuration
#define DEVICE_I2S_NUM          I2S_NUM_0
#define DEVICE_I2S_MODE         (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX)
#define DEVICE_I2S_CHANNEL_FMT  I2S_CHANNEL_FMT_RIGHT_LEFT  // Stereo
#define DEVICE_I2S_COMM_FMT     I2S_COMM_FORMAT_STAND_I2S

// Audio codec I2C configuration
#define DEVICE_CODEC_I2C_ADDR   0x18    // AIC3204 typical address

// Default audio settings
#define DEVICE_DEFAULT_MIC_GAIN     24  // 24dB (moderate gain)
#define DEVICE_DEFAULT_SPEAKER_VOL  60  // 60%

// Physical control configuration
#define DEVICE_BUTTON_COUNT     1       // Single multi-function button
#define DEVICE_HAS_ROTARY_ENCODER true  // For volume control

#endif // DEVICE_NABU_CASA_CONFIG_H
