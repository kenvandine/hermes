#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include <Arduino.h>

// Forward declaration for state machine
enum class AppState;

/**
 * Display Capabilities Structure
 *
 * Describes what display features are available on the device
 */
struct DisplayCapabilities {
    bool hasFullUI;      // Full LVGL graphical UI support
    bool hasLEDRing;     // LED ring for status indicators
    bool hasTouchScreen; // Touch screen input
    uint16_t width;      // Display width in pixels (0 if no display)
    uint16_t height;     // Display height in pixels (0 if no display)
    uint8_t ledCount;    // Number of LEDs in ring (0 if no LED ring)
};

/**
 * Hardware Abstraction Layer for Display
 *
 * Provides a unified interface for different display hardware.
 * Supports both full graphical displays (AMOLED) and simple LED indicators.
 */
class HALDisplay {
public:
    virtual ~HALDisplay() = default;

    /**
     * Initialize the display hardware
     * @return true if successful
     */
    virtual bool begin() = 0;

    /**
     * Update display (called in main loop)
     * Handles animations, touch events, etc.
     */
    virtual void update() = 0;

    /**
     * Set display brightness
     * @param brightness Brightness level 0-100
     */
    virtual void setBrightness(uint8_t brightness) = 0;

    /**
     * Show current application state
     * @param state Current state (IDLE, LISTENING, CALLING, etc.)
     */
    virtual void showState(AppState state) = 0;

    /**
     * Show a text message
     * @param message Message to display
     * @param durationMs How long to show (0 = until cleared)
     */
    virtual void showMessage(const String& message, uint32_t durationMs = 0) = 0;

    /**
     * Show error message
     * @param error Error message to display
     */
    virtual void showError(const String& error) = 0;

    /**
     * Get display capabilities
     * @return Capabilities structure
     */
    virtual DisplayCapabilities getCapabilities() const = 0;

    /**
     * Update device list (if applicable)
     * For displays with full UI showing available devices
     */
    virtual void updateDeviceList() = 0;

    /**
     * Update call information (if applicable)
     * For displays showing call duration, audio levels, etc.
     */
    virtual void updateCallInfo() = 0;
};

#endif // HAL_DISPLAY_H
