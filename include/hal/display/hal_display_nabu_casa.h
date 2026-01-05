#ifndef HAL_DISPLAY_NABU_CASA_H
#define HAL_DISPLAY_NABU_CASA_H

#include "hal/hal_display.h"
#include <FastLED.h>

/**
 * Nabu Casa Display HAL Implementation
 *
 * Uses 12-LED WS2812B ring for visual state indicators:
 * - IDLE: Soft breathing white/blue
 * - LISTENING: Pulsing blue
 * - PROCESSING: Spinning animation
 * - CALLING: Solid green
 * - RINGING: Blinking yellow/green
 * - ERROR: Red flash
 */
class HALDisplayNabuCasa : public HALDisplay {
public:
    HALDisplayNabuCasa();
    virtual ~HALDisplayNabuCasa() = default;

    // HALDisplay interface implementation
    bool begin() override;
    void update() override;
    void setBrightness(uint8_t brightness) override;
    void showState(AppState state) override;
    void showMessage(const String& message, uint32_t durationMs = 0) override;
    void showError(const String& error) override;
    DisplayCapabilities getCapabilities() const override;
    void updateDeviceList() override;
    void updateCallInfo() override;

    // Volume display (Nabu Casa specific)
    void showVolume(uint8_t volume, bool muted);

private:
    DisplayCapabilities capabilities_;
    uint8_t brightness_;

    // LED ring state
    AppState currentState_;
    uint32_t animationPhase_;
    uint32_t lastUpdateMs_;

    // Volume display state
    bool showingVolume_;
    uint8_t displayVolume_;
    bool displayMuted_;
    uint32_t volumeDisplayEndMs_;

    // LED buffer (initialized by FastLED)
    CRGB leds_[12];

    // Animation helpers
    void updateAnimation();
    void setAllLEDs(CRGB color);
    void setLED(uint8_t index, CRGB color);
    CRGB getStateColor(AppState state);
};

#endif // HAL_DISPLAY_NABU_CASA_H
