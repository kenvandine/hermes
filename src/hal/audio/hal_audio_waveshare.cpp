/**
 * Waveshare Audio HAL Implementation
 *
 * Wraps I2SManager which handles ES8311 codec + I2S communication
 */

#include "hal/audio/hal_audio_waveshare.h"
#include "audio/i2s_manager.h"
#include "devices/waveshare/device_config.h"
#include <Arduino.h>

HALAudioWaveshare::HALAudioWaveshare()
    : codec_(nullptr)
    , i2sManager_(nullptr)
    , sampleRate_(DEVICE_SAMPLE_RATE)
    , micReady_(false)
    , speakerReady_(false) {
}

HALAudioWaveshare::~HALAudioWaveshare() {
    // Destructor must be defined in .cpp to allow unique_ptr with incomplete types
}

bool HALAudioWaveshare::begin(uint32_t sampleRate) {
    sampleRate_ = sampleRate;

    Serial.println("[HAL-Audio-Waveshare] Initializing ES8311 codec + I2S...");

    // Create I2SManager instance (which contains ES8311 codec)
    i2sManager_ = std::make_unique<I2SManager>();

    // Initialize ES8311 codec
    if (!i2sManager_->begin(sampleRate_)) {
        Serial.println("[HAL-Audio-Waveshare] Failed to initialize codec");
        return false;
    }

    // Initialize microphone and speaker
    if (!beginMicrophone(sampleRate_)) {
        Serial.println("[HAL-Audio-Waveshare] Failed to initialize microphone");
        return false;
    }

    if (!beginSpeaker(sampleRate_)) {
        Serial.println("[HAL-Audio-Waveshare] Failed to initialize speaker");
        return false;
    }

    Serial.println("[HAL-Audio-Waveshare] ✓ Audio HAL initialized");
    return true;
}

bool HALAudioWaveshare::beginMicrophone(uint32_t sampleRate) {
    if (!i2sManager_) {
        return false;
    }

    if (!i2sManager_->beginMicrophone(sampleRate)) {
        return false;
    }

    micReady_ = true;
    return true;
}

bool HALAudioWaveshare::beginSpeaker(uint32_t sampleRate) {
    if (!i2sManager_) {
        return false;
    }

    if (!i2sManager_->beginSpeaker(sampleRate)) {
        return false;
    }

    speakerReady_ = true;
    return true;
}

size_t HALAudioWaveshare::readMicrophone(int16_t* buffer, size_t sampleCount) {
    if (!micReady_ || !i2sManager_) {
        return 0;
    }

    return i2sManager_->readMicrophone(buffer, sampleCount);
}

size_t HALAudioWaveshare::writeSpeaker(const int16_t* buffer, size_t sampleCount) {
    if (!speakerReady_ || !i2sManager_) {
        return 0;
    }

    return i2sManager_->writeSpeaker(buffer, sampleCount);
}

void HALAudioWaveshare::setVolume(uint8_t volume) {
    if (i2sManager_) {
        i2sManager_->setVolume(volume);
    }
}

void HALAudioWaveshare::setMicGain(uint8_t gainStep) {
    if (i2sManager_) {
        i2sManager_->setMicGain(gainStep);
    }
}

void HALAudioWaveshare::mute(bool enabled) {
    if (i2sManager_) {
        i2sManager_->mute(enabled);
    }
}

uint32_t HALAudioWaveshare::getSampleRate() const {
    return sampleRate_;
}

uint8_t HALAudioWaveshare::getChannelCount() const {
    return DEVICE_AUDIO_CHANNELS;
}

bool HALAudioWaveshare::isMicrophoneReady() const {
    return micReady_;
}

bool HALAudioWaveshare::isSpeakerReady() const {
    return speakerReady_;
}
