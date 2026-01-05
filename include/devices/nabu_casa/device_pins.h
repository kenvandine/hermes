#ifndef DEVICE_NABU_CASA_PINS_H
#define DEVICE_NABU_CASA_PINS_H

/**
 * Nabu Casa Voice Preview Edition Pin Definitions
 *
 * Based on official ESPHome configuration and schematic:
 * https://github.com/esphome/home-assistant-voice-pe
 * https://github.com/NabuCasa/support/blob/main/static/docs/voice/home_assistant_voice_pe_schematic_v1.0_241009.pdf
 *
 * The XMOS XU316 handles microphone array processing (echo cancellation,
 * noise suppression, auto gain) and communicates with ESP32-S3 via dual I2S.
 */

// ============================================================================
// I2S Audio Pins (Dual I2S buses for input and output)
// ============================================================================

// I2S Input (Microphone from XMOS) - 16kHz, stereo, 32-bit
#define PIN_I2S_MIC_LRCLK       14      // Word Select / LRCLK (input)
#define PIN_I2S_MIC_BCLK        13      // Bit Clock (input)
#define PIN_I2S_MIC_DIN         15      // Serial Data IN from XMOS

// I2S Output (Speaker to AIC3204) - 48kHz, stereo, 32-bit
#define PIN_I2S_SPK_LRCLK       7       // Word Select / LRCLK (output)
#define PIN_I2S_SPK_BCLK        8       // Bit Clock (output)
#define PIN_I2S_SPK_DOUT        10      // Serial Data OUT to AIC3204

// No MCLK - XMOS generates clocks
#define PIN_I2S_MCLK            -1      // Not used (XMOS is I2S master)

// ============================================================================
// I2C Bus (for AIC3204 codec control and XMOS communication)
// ============================================================================

#define PIN_I2C_SDA             5       // I2C Data (400kHz)
#define PIN_I2C_SCL             6       // I2C Clock

// ============================================================================
// LED Ring Pins (WS2812B - 12 RGB LEDs)
// ============================================================================

#define PIN_LED_RING_DATA       21      // LED data pin (WS2812B) - Correct per ESPHome
#define PIN_LED_RING_POWER      45      // LED power supply enable

// ============================================================================
// Audio Control Pins
// ============================================================================

#define PIN_SPEAKER_AMP_ENABLE  47      // Internal speaker amplifier enable
#define PIN_VOICE_KIT_RESET     4       // XMOS reset pin

// ============================================================================
// Physical Control Pins (TODO: Verify these from schematic)
// ============================================================================

// Multi-function button
#define PIN_BUTTON_ACTION       -1      // Main action button (TODO)

// Rotary encoder for volume control
#define PIN_ROTARY_A            -1      // Rotary encoder A (TODO)
#define PIN_ROTARY_B            -1      // Rotary encoder B (TODO)
#define PIN_ROTARY_BUTTON       -1      // Rotary encoder button (TODO)

// Hardware mute switch
#define PIN_MUTE_SWITCH         -1      // Mute switch (TODO)

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
