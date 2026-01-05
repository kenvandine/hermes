#ifndef DEVICE_WAVESHARE_PINS_H
#define DEVICE_WAVESHARE_PINS_H

/**
 * Waveshare ESP32-S3-Touch-AMOLED-1.8" Pin Definitions
 *
 * These pins are fixed by the board design and cannot be changed.
 */

// ============================================================================
// I2S Audio Pins (ES8311 Codec)
// ============================================================================

// ES8311 uses same I2S bus for both ADC (microphone) and DAC (speaker)
#define PIN_I2S_MCLK            42      // Master Clock (shared)
#define PIN_I2S_BCLK            9       // Bit Clock (shared)
#define PIN_I2S_WS              45      // Word Select / LRCLK (shared)
#define PIN_I2S_SD_IN           10      // Serial Data IN (microphone)
#define PIN_I2S_SD_OUT          8       // Serial Data OUT (speaker)

// ============================================================================
// I2C Bus (shared by Touch and Audio Codec)
// ============================================================================

#define PIN_I2C_SDA             15      // I2C Data
#define PIN_I2C_SCL             14      // I2C Clock

// ============================================================================
// Display Pins (SH8601 AMOLED via QSPI)
// ============================================================================

#define PIN_DISPLAY_CS          12      // Chip Select
#define PIN_DISPLAY_RST         -1      // Reset (not used)
#define PIN_DISPLAY_SDA0        4       // QSPI Data 0
#define PIN_DISPLAY_SDA1        5       // QSPI Data 1
#define PIN_DISPLAY_SDA2        6       // QSPI Data 2
#define PIN_DISPLAY_SDA3        7       // QSPI Data 3
#define PIN_DISPLAY_SCL         11      // QSPI Clock

// ============================================================================
// Touch Screen Pins (FT3168)
// ============================================================================

#define PIN_TOUCH_INT           21      // Touch interrupt
// Touch uses I2C (SDA=15, SCL=14) - see I2C section above

// ============================================================================
// Physical Controls (Not available on this device)
// ============================================================================

#define PIN_BUTTON_ACCEPT       -1      // Not used
#define PIN_BUTTON_REJECT       -1      // Not used
#define PIN_BUTTON_HANGUP       -1      // Not used
#define PIN_VOLUME_DIAL         -1      // Not used
#define PIN_MUTE_SWITCH         -1      // Not used

// ============================================================================
// Status LED (Optional)
// ============================================================================

#define PIN_LED_STATUS          -1      // Not used

#endif // DEVICE_WAVESHARE_PINS_H
