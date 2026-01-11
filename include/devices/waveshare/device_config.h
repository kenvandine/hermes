#ifndef DEVICE_WAVESHARE_CONFIG_H
#define DEVICE_WAVESHARE_CONFIG_H

/**
 * Waveshare ESP32-S3-Touch-AMOLED-1.8" Device Configuration
 *
 * Hardware:
 * - ESP32-S3 (16MB Flash, 8MB PSRAM)
 * - ES8311 Audio Codec (I2S, I2C control)
 * - 1.8" AMOLED Display (368x448, QSPI)
 * - FT3168 Capacitive Touch Controller
 * - Built-in microphone and speaker
 */

// Device identification
#define DEVICE_NAME             "Waveshare ESP32-S3 AMOLED"
#define DEVICE_MODEL            "ESP32-S3-Touch-AMOLED-1.8"
#define DEVICE_MANUFACTURER     "Waveshare"

// Hardware capabilities
#define DEVICE_HAS_DISPLAY      true
#define DEVICE_HAS_TOUCHSCREEN  true
#define DEVICE_HAS_LED_RING     false
#define DEVICE_HAS_PHYSICAL_BUTTONS false
#define DEVICE_HAS_VOLUME_DIAL  false
#define DEVICE_HAS_MUTE_SWITCH  false

// Audio configuration
#define DEVICE_AUDIO_CODEC      "ES8311"
#define DEVICE_SAMPLE_RATE      16000   // 16kHz for voice
#define DEVICE_BITS_PER_SAMPLE  16
#define DEVICE_AUDIO_CHANNELS   1       // Mono
#define DEVICE_HAS_ECHO_CANCEL  false   // No hardware echo cancellation
#define DEVICE_MIC_COUNT        1       // Single microphone
#define DEVICE_NEEDS_BIDIRECTIONAL_I2S false  // ES8311 doesn't need silence fed to speaker

// Display configuration
#define DEVICE_DISPLAY_TYPE     "AMOLED"
#define DEVICE_DISPLAY_CONTROLLER "SH8601"
#define DEVICE_DISPLAY_WIDTH    368
#define DEVICE_DISPLAY_HEIGHT   448
#define DEVICE_DISPLAY_ROTATION 0       // Portrait
#define DEVICE_COLOR_DEPTH      16      // 16-bit color

// Touch configuration
#define DEVICE_TOUCH_CONTROLLER "FT3168"
#define DEVICE_TOUCH_I2C_ADDR   0x15

// I2S configuration
#define DEVICE_I2S_NUM          I2S_NUM_1
#define DEVICE_I2S_MODE         (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX)
#define DEVICE_I2S_CHANNEL_FMT  I2S_CHANNEL_FMT_ONLY_LEFT
#define DEVICE_I2S_COMM_FMT     I2S_COMM_FORMAT_STAND_I2S

// Audio codec I2C configuration
#define DEVICE_CODEC_I2C_ADDR   0x18    // ES8311 I2C address

// Default audio settings
#define DEVICE_DEFAULT_MIC_GAIN     36  // 36dB (max stable for ES8311)
#define DEVICE_DEFAULT_SPEAKER_VOL  70  // 70%

#endif // DEVICE_WAVESHARE_CONFIG_H
