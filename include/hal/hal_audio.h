#ifndef HAL_AUDIO_H
#define HAL_AUDIO_H

#include <Arduino.h>
#include <cstdint>
#include <cstddef>

/**
 * Hardware Abstraction Layer for Audio
 *
 * Provides a unified interface for different audio codec hardware.
 * Implementations handle device-specific I2S configuration and codec control.
 */
class HALAudio {
public:
    virtual ~HALAudio() = default;

    /**
     * Initialize the audio codec
     * @param sampleRate Audio sample rate in Hz (e.g., 16000, 48000)
     * @return true if successful
     */
    virtual bool begin(uint32_t sampleRate) = 0;

    /**
     * Initialize microphone input
     * @param sampleRate Sample rate in Hz
     * @return true if successful
     */
    virtual bool beginMicrophone(uint32_t sampleRate) = 0;

    /**
     * Initialize speaker output
     * @param sampleRate Sample rate in Hz
     * @return true if successful
     */
    virtual bool beginSpeaker(uint32_t sampleRate) = 0;

    /**
     * Read audio samples from microphone
     * @param buffer Destination buffer for int16_t samples
     * @param sampleCount Number of samples to read
     * @return Number of samples actually read
     */
    virtual size_t readMicrophone(int16_t* buffer, size_t sampleCount) = 0;

    /**
     * Write audio samples to speaker
     * @param buffer Source buffer of int16_t samples
     * @param sampleCount Number of samples to write
     * @return Number of samples actually written
     */
    virtual size_t writeSpeaker(const int16_t* buffer, size_t sampleCount) = 0;

    /**
     * Set speaker volume
     * @param volume Volume level 0-100
     */
    virtual void setVolume(uint8_t volume) = 0;

    /**
     * Set microphone gain
     * @param gainStep Gain step (device-specific mapping)
     */
    virtual void setMicGain(uint8_t gainStep) = 0;

    /**
     * Mute/unmute speaker
     * @param enabled true to mute, false to unmute
     */
    virtual void mute(bool enabled) = 0;

    /**
     * Get configured sample rate
     * @return Sample rate in Hz
     */
    virtual uint32_t getSampleRate() const = 0;

    /**
     * Get number of audio channels
     * @return 1 for mono, 2 for stereo
     */
    virtual uint8_t getChannelCount() const = 0;

    /**
     * Check if microphone is ready
     * @return true if initialized and ready
     */
    virtual bool isMicrophoneReady() const = 0;

    /**
     * Check if speaker is ready
     * @return true if initialized and ready
     */
    virtual bool isSpeakerReady() const = 0;
};

#endif // HAL_AUDIO_H
