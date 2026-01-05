#ifndef HAL_AUDIO_WAVESHARE_H
#define HAL_AUDIO_WAVESHARE_H

#include "hal/hal_audio.h"
#include <memory>

// Forward declarations
class ES8311;
class I2SManager;

/**
 * Waveshare Audio HAL Implementation
 *
 * Wraps ES8311 codec and I2S manager for the Waveshare ESP32-S3 AMOLED board
 */
class HALAudioWaveshare : public HALAudio {
public:
    HALAudioWaveshare();
    virtual ~HALAudioWaveshare();  // Defined in .cpp to allow incomplete types in unique_ptr

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
    std::unique_ptr<ES8311> codec_;
    std::unique_ptr<I2SManager> i2sManager_;
    uint32_t sampleRate_;
    bool micReady_;
    bool speakerReady_;
};

#endif // HAL_AUDIO_WAVESHARE_H
