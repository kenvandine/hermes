#ifndef DEVICE_NABU_CASA_PINS_H
#define DEVICE_NABU_CASA_PINS_H

/**
 * Nabu Casa Voice Preview Edition Pin Definitions
 *
 * NOTE: These pin assignments are placeholders and need to be verified
 * against the actual hardware schematic. The Nabu Casa Voice PE uses
 * XMOS XU316 for audio processing which handles the I2S communication
 * between microphones and the ESP32-S3.
 *
 * TODO: Update with actual pin mappings from hardware documentation
 */

// ============================================================================
// I2S Audio Pins (XMOS XU316 <-> ESP32-S3)
// ============================================================================

// The XMOS processor handles microphone array processing and presents
// processed audio to ESP32-S3 via I2S
#define PIN_I2S_MCLK            0       // Master Clock (TBD)
#define PIN_I2S_BCLK            1       // Bit Clock (TBD)
#define PIN_I2S_WS              2       // Word Select / LRCLK (TBD)
#define PIN_I2S_SD_IN           3       // Serial Data IN from XMOS (TBD)
#define PIN_I2S_SD_OUT          4       // Serial Data OUT to AIC3204 (TBD)

// ============================================================================
// I2C Bus (for AIC3204 codec control and possibly XMOS)
// ============================================================================

#define PIN_I2C_SDA             5       // I2C Data (TBD)
#define PIN_I2C_SCL             6       // I2C Clock (TBD)

// ============================================================================
// LED Ring Pins (WS2812B)
// ============================================================================

#define PIN_LED_RING_DATA       7       // LED data pin (TBD)
#define PIN_LED_RING_POWER      -1      // Optional power control (TBD)

// ============================================================================
// Physical Control Pins
// ============================================================================

// Multi-function button
#define PIN_BUTTON_ACTION       8       // Main action button (TBD)

// Rotary encoder for volume control
#define PIN_ROTARY_A            9       // Rotary encoder A (TBD)
#define PIN_ROTARY_B            10      // Rotary encoder B (TBD)
#define PIN_ROTARY_BUTTON       11      // Rotary encoder button (TBD)

// Hardware mute switch
#define PIN_MUTE_SWITCH         12      // Mute switch (TBD)

// ============================================================================
// Not Used (No display/touchscreen)
// ============================================================================

#define PIN_DISPLAY_CS          -1      // Not used
#define PIN_DISPLAY_RST         -1      // Not used
#define PIN_TOUCH_INT           -1      // Not used

// ============================================================================
// Status/Debug
// ============================================================================

#define PIN_LED_STATUS          -1      // Separate status LED if present (TBD)

// ============================================================================
// XMOS Communication (if direct control needed)
// ============================================================================

// XMOS XU316 may use additional pins for configuration/reset
#define PIN_XMOS_RESET          -1      // XMOS reset (TBD)
#define PIN_XMOS_MODE           -1      // XMOS mode select (TBD)

#endif // DEVICE_NABU_CASA_PINS_H
