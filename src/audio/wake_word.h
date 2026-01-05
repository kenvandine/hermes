#ifndef WAKE_WORD_H
#define WAKE_WORD_H

#include <Arduino.h>

/**
 * WakeWord
 *
 * Integrates Edge Impulse wake word detection.
 * Continuously processes audio to detect the wake phrase ("Hey Hermes").
 *
 * To use this class:
 * 1. Train a wake word model on Edge Impulse (https://edgeimpulse.com)
 * 2. Export as "Arduino library"
 * 3. Add the exported library to lib/ directory
 * 4. Uncomment the Edge Impulse includes below
 * 5. Set WAKE_WORD_ENABLED=true in config.h
 *
 * Features:
 * - Continuous inference on audio stream
 * - Configurable detection threshold
 * - Debouncing to prevent multiple triggers
 * - Low CPU overhead when properly trained
 */
class WakeWord {
public:
    WakeWord();
    ~WakeWord();

    /**
     * Initialize wake word detector
     * @param sampleRate Sample rate in Hz (must match Edge Impulse model)
     * @param threshold Detection threshold (0.0-1.0, higher = stricter)
     * @return true if successful
     */
    bool begin(uint32_t sampleRate = 16000, float threshold = 0.8);

    /**
     * Stop wake word detector
     */
    void stop();

    /**
     * Process audio samples and check for wake word
     * @param samples Audio samples (int16_t)
     * @param sampleCount Number of samples
     * @return true if wake word detected
     */
    bool process(const int16_t* samples, size_t sampleCount);

    /**
     * Check if wake word was detected
     * This method debounces multiple detections
     */
    bool isDetected();

    /**
     * Reset detection state
     */
    void reset();

    /**
     * Clear the rolling buffer (fill with zeros)
     * Use this before recording to start with fresh audio
     */
    void clearBuffer();

    /**
     * Set detection threshold
     * @param threshold 0.0 (always detect) to 1.0 (never detect)
     */
    void setThreshold(float threshold);

    /**
     * Get last detection confidence (0.0-1.0)
     */
    float getLastConfidence() const;

    /**
     * Get inference time (milliseconds)
     */
    unsigned long getInferenceTime() const;

    /**
     * Check if initialized
     */
    bool isReady() const;

    /**
     * Enable/disable wake word detection
     */
    void enable(bool enabled);

    /**
     * Check if enabled
     */
    bool isEnabled() const;

    /**
     * Save current rolling buffer to SPIFFS as WAV file for debugging
     * @param filename Path to save WAV file (e.g. "/recording.wav")
     * @return true if successful
     */
    bool saveBufferToWAV(const char* filename);

    /**
     * Get pointer to rolling buffer for debugging
     * @param size Output parameter for buffer size
     * @return Pointer to buffer (or nullptr if not initialized)
     */
    const int16_t* getBuffer(size_t* size) const;

private:
    bool initialized;
    bool enabled;
    uint32_t sampleRate;
    float threshold;
    float lastConfidence;
    unsigned long lastDetectionTime;
    unsigned long debounceMs;
    unsigned long inferenceTimeMs;

    // Edge Impulse inference buffer
    float* inferenceBuffer;
    size_t inferenceBufferSize;

    // Temporary buffer for sample conversion
    float* sampleBuffer;
    size_t sampleBufferSize;

    // Rolling buffer for accumulating audio samples
    int16_t* rollingBuffer;
    size_t rollingBufferSize;
    size_t rollingBufferPos;

    // Process inference and check for wake word
    bool runInference(const int16_t* samples, size_t sampleCount);

    // Convert int16_t samples to float
    void convertSamples(const int16_t* input, float* output, size_t count);
};

#endif // WAKE_WORD_H
