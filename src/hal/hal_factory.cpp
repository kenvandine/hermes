/**
 * HAL Factory Implementation
 */

#include "hal/hal_factory.h"

// Include device-specific implementations
#if defined(DEVICE_WAVESHARE)
    #include "hal/audio/hal_audio_waveshare.h"
    #include "hal/display/hal_display_waveshare.h"
    #include "hal/controls/hal_controls_waveshare.h"
    #include "devices/waveshare/device_config.h"
#elif defined(DEVICE_NABU_CASA)
    #include "hal/audio/hal_audio_nabu_casa.h"
    #include "hal/display/hal_display_nabu_casa.h"
    #include "hal/controls/hal_controls_nabu_casa.h"
    #include "devices/nabu_casa/device_config.h"
#else
    #error "No device specified! Define DEVICE_WAVESHARE or DEVICE_NABU_CASA in build flags"
#endif

std::unique_ptr<HALAudio> HALFactory::createAudio() {
#if defined(DEVICE_WAVESHARE)
    return std::make_unique<HALAudioWaveshare>();
#elif defined(DEVICE_NABU_CASA)
    return std::make_unique<HALAudioNabuCasa>();
#endif
}

std::unique_ptr<HALDisplay> HALFactory::createDisplay() {
#if defined(DEVICE_WAVESHARE)
    return std::make_unique<HALDisplayWaveshare>();
#elif defined(DEVICE_NABU_CASA)
    return std::make_unique<HALDisplayNabuCasa>();
#endif
}

std::unique_ptr<HALControls> HALFactory::createControls() {
#if defined(DEVICE_WAVESHARE)
    return std::make_unique<HALControlsWaveshare>();
#elif defined(DEVICE_NABU_CASA)
    return std::make_unique<HALControlsNabuCasa>();
#endif
}

const char* HALFactory::getDeviceName() {
#if defined(DEVICE_WAVESHARE)
    return DEVICE_NAME;
#elif defined(DEVICE_NABU_CASA)
    return DEVICE_NAME;
#else
    return "Unknown Device";
#endif
}
