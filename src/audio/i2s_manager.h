#ifndef I2S_MANAGER_H
#define I2S_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "ES8311.h"

/**
 * I2SManager
 *
 * Manages I2S hardware for audio input (microphone) and output (speaker).
 * Supports ESP32-S3 with I2S port.
 *
 * Features:
 * - Configurable sample rate and bit depth
 * - DMA buffering for efficient transfers
 * - Volume control
 * - Single I2S port for full duplex audio (Microphone and Speaker)
 */
class I2SManager {
public:
    I2SManager();
    ~I2SManager();

    /**
     * Initialize audio system (ES8311 codec + I2S)
     * Must be called before beginMicrophone or beginSpeaker
     * @param sampleRate Sample rate in Hz (e.g., 16000)
     * @return true if successful
     */
    bool begin(uint32_t sampleRate = 16000);

    /**
     * Enable microphone input
     * @param sampleRate Sample rate in Hz (e.g., 16000)
     * @param bitsPerSample Bits per sample (16 or 32)
     * @return true if successful
     */
    bool beginMicrophone(uint32_t sampleRate = 16000, i2s_bits_per_sample_t bitsPerSample = I2S_BITS_PER_SAMPLE_16BIT);

    /**
     * Enable speaker output
     * @param sampleRate Sample rate in Hz (e.g., 16000)
     * @param bitsPerSample Bits per sample (16 or 32)
     * @return true if successful
     */
    bool beginSpeaker(uint32_t sampleRate = 16000, i2s_bits_per_sample_t bitsPerSample = I2S_BITS_PER_SAMPLE_16BIT);

    /**
     * Stop microphone
     */
    void stopMicrophone();

    /**
     * Stop speaker
     */
    void stopSpeaker();

    /**
     * Read audio samples from microphone
     * @param buffer Destination buffer for samples (int16_t)
     * @param sampleCount Number of samples to read
     * @return Number of samples actually read
     */
    size_t readMicrophone(int16_t* buffer, size_t sampleCount);

    /**
     * Write audio samples to speaker
     * @param buffer Source buffer of samples (int16_t)
     * @param sampleCount Number of samples to write
     * @return Number of samples actually written
     */
    size_t writeSpeaker(const int16_t* buffer, size_t sampleCount);

    /**
     * Set speaker volume (0-100)
     */
    void setVolume(uint8_t volume);

    /**
     * Get current volume (0-100)
     */
    uint8_t getVolume() const;

    /**
     * Mute/unmute speaker
     */
    void mute(bool enabled);

    /**
     * Check if microphone is initialized
     */
    bool isMicrophoneReady() const;

    /**
     * Check if speaker is initialized
     */
    bool isSpeakerReady() const;

    /**
     * Get microphone sample rate
     */
    uint32_t getMicrophoneSampleRate() const;

    /**
     * Get speaker sample rate
     */
    uint32_t getSpeakerSampleRate() const;

private:
    ES8311 codec;  // Audio codec for built-in mic/speaker

    i2s_port_t i2sPort;

    uint32_t currentSampleRate;

    bool codecReady;
    bool i2sReady;
    bool micEnabled;
    bool speakerEnabled;
    bool muted;

    uint8_t volume;  // 0-100

    // Apply volume to samples
    void applyVolume(int16_t* buffer, size_t sampleCount);
};

#endif // I2S_MANAGER_H
