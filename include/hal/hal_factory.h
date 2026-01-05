#ifndef HAL_FACTORY_H
#define HAL_FACTORY_H

#include "hal/hal_audio.h"
#include "hal/hal_display.h"
#include "hal/hal_controls.h"
#include <memory>

/**
 * HAL Factory
 *
 * Creates device-specific HAL implementations based on compile-time
 * build flags (DEVICE_WAVESHARE or DEVICE_NABU_CASA).
 *
 * Usage:
 *   auto audio = HALFactory::createAudio();
 *   audio->begin(16000);
 */
class HALFactory {
public:
    /**
     * Create audio HAL instance for configured device
     * @return Unique pointer to device-specific audio HAL
     */
    static std::unique_ptr<HALAudio> createAudio();

    /**
     * Create display HAL instance for configured device
     * @return Unique pointer to device-specific display HAL
     */
    static std::unique_ptr<HALDisplay> createDisplay();

    /**
     * Create controls HAL instance for configured device
     * @return Unique pointer to device-specific controls HAL
     */
    static std::unique_ptr<HALControls> createControls();

    /**
     * Get device name (for logging/debugging)
     * @return Device name string
     */
    static const char* getDeviceName();

private:
    HALFactory() = delete;  // Static class, no instances
};

#endif // HAL_FACTORY_H
