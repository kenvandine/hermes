#ifndef HAL_CONTROLS_H
#define HAL_CONTROLS_H

#include <Arduino.h>
#include <functional>

/**
 * Control Events
 *
 * Unified control events that can be triggered by touch screen,
 * physical buttons, or other input methods
 */
enum class ControlEvent {
    NONE,

    // Call controls
    ACCEPT_CALL,
    REJECT_CALL,
    HANG_UP,

    // Volume controls
    VOLUME_UP,
    VOLUME_DOWN,
    VOLUME_SET,      // For rotary dial

    // Mode controls
    MUTE_TOGGLE,
    WAKE_WORD_TRIGGER,

    // Navigation
    SETTINGS,
    BACK,
    CANCEL,
    SELECT_DEVICE,   // For device selection in UI

    // AI controls
    AI_QUERY_START,
    AI_QUERY_CANCEL
};

/**
 * Control Event Data
 *
 * Additional data associated with control events
 */
struct ControlEventData {
    ControlEvent event;
    int32_t value;      // For events with values (e.g., volume level, device index)
    void* userData;     // For device-specific data

    ControlEventData() : event(ControlEvent::NONE), value(0), userData(nullptr) {}
    ControlEventData(ControlEvent e) : event(e), value(0), userData(nullptr) {}
    ControlEventData(ControlEvent e, int32_t v) : event(e), value(v), userData(nullptr) {}
};

/**
 * Hardware Abstraction Layer for Input Controls
 *
 * Provides a unified interface for different input methods:
 * - Touch screen (Waveshare)
 * - Physical buttons, dial, switches (Nabu Casa)
 */
class HALControls {
public:
    typedef std::function<void(const ControlEventData&)> ControlCallback;

    virtual ~HALControls() = default;

    /**
     * Initialize control hardware
     * @return true if successful
     */
    virtual bool begin() = 0;

    /**
     * Update controls (called in main loop)
     * Handles button debouncing, touch events, etc.
     */
    virtual void update() = 0;

    /**
     * Register callback for control events
     * @param callback Function to call when control event occurs
     */
    virtual void registerCallback(ControlCallback callback) = 0;

    /**
     * Check if device has touch screen
     * @return true if touch screen is available
     */
    virtual bool hasTouchScreen() const = 0;

    /**
     * Check if device has physical controls
     * @return true if physical buttons/dial/switches are available
     */
    virtual bool hasPhysicalControls() const = 0;

    /**
     * Enable/disable controls
     * @param enabled true to enable, false to disable
     */
    virtual void setEnabled(bool enabled) = 0;

    /**
     * Get current volume level (from dial if applicable)
     * @return Volume level 0-100, or -1 if not applicable
     */
    virtual int getVolumeDial() const = 0;

    /**
     * Get mute switch state (if applicable)
     * @return true if muted, false if not muted or no switch
     */
    virtual bool getMuteSwitch() const = 0;
};

#endif // HAL_CONTROLS_H
