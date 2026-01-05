#ifndef HAL_AUDIO_NABU_CASA_H
#define HAL_AUDIO_NABU_CASA_H

#include "hal/hal_audio.h"

/**
 * Nabu Casa Audio HAL Implementation
 *
 * Implements XMOS XU316 + TI AIC3204 audio system:
 * - Dual I2S buses (16kHz mic, 48kHz speaker)
 * - Stereo to mono conversion
 * - Hardware echo cancellation (XMOS)
 * - Hardware noise suppression (XMOS)
 */
class HALAudioNabuCasa : public HALAudio {
public:
    HALAudioNabuCasa();
    virtual ~HALAudioNabuCasa();

    // HALAudio interface implementation
    bool begin(uint32_t sampleRate) override;
    bool beginMicrophone(uint32_t sampleRate) override;
    bool beginSpeaker(uint32_t sampleRate) override;
    size_t readMicrophone(int16_t* buffer, size_t sampleCount) override;
    size_t writeSpeaker(const int16_t* buffer, size_t sampleCount) override;
    void setVolume(uint8_t volume) override;
    void setMicGain(uint8_t gainStep) override;
    void mute(bool enabled) override;
    uint32_t getSampleRate() const override;
    uint8_t getChannelCount() const override;
    bool isMicrophoneReady() const override;
    bool isSpeakerReady() const override;

private:
    uint32_t sampleRate_;
    uint8_t volume_;
    uint8_t micGain_;
    bool muted_;
    bool micReady_;
    bool speakerReady_;
};

#endif // HAL_AUDIO_NABU_CASA_H
