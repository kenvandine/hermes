#ifndef SPEECH_RECOGNIZER_H
#define SPEECH_RECOGNIZER_H

#include <Arduino.h>
#include <functional>

/**
 * SpeechRecognizer
 *
 * Integrates Edge Impulse keyword spotting for voice commands.
 * Recognizes command keywords and room names.
 *
 * To use this class:
 * 1. Train a keyword spotting model on Edge Impulse with classes:
 *    - Command keywords: "drop_in", "call", "hang_up", "cancel"
 *    - Room names: "kitchen", "living_room", "bedroom", etc.
 *    - "noise" class for background/unknown audio
 * 2. Export as "Arduino library"
 * 3. Add the exported library to lib/ directory
 * 4. Uncomment the Edge Impulse includes below
 * 5. Set VOICE_COMMANDS_ENABLED=true in config.h
 *
 * Features:
 * - Continuous inference on audio stream
 * - Multi-class keyword spotting (commands + rooms)
 * - Configurable detection threshold
 * - Callback on keyword detection
 */
class SpeechRecognizer {
public:
    /**
     * Keyword detection callback
     * @param keyword Detected keyword (e.g., "kitchen", "drop_in")
     * @param confidence Detection confidence (0.0-1.0)
     */
    typedef std::function<void(const String& keyword, float confidence)> KeywordCallback;

    SpeechRecognizer();
    ~SpeechRecognizer();

    /**
     * Initialize speech recognizer
     * @param sampleRate Sample rate in Hz (must match Edge Impulse model)
     * @param threshold Detection threshold (0.0-1.0, higher = stricter)
     * @return true if successful
     */
    bool begin(uint32_t sampleRate = 16000, float threshold = 0.8);

    /**
     * Stop speech recognizer
     */
    void stop();

    /**
     * Process audio samples and check for keywords
     * @param samples Audio samples (int16_t)
     * @param sampleCount Number of samples
     * @return true if keyword detected (callback will be invoked)
     */
    bool process(const int16_t* samples, size_t sampleCount);

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
     * Get last detected keyword
     */
    String getLastKeyword() const;

    /**
     * Get inference time (milliseconds)
     */
    unsigned long getInferenceTime() const;

    /**
     * Check if initialized
     */
    bool isReady() const;

    /**
     * Enable/disable recognition
     */
    void enable(bool enabled);

    /**
     * Check if enabled
     */
    bool isEnabled() const;

    /**
     * Register callback for keyword detection
     */
    void onKeywordDetected(KeywordCallback callback);

private:
    bool initialized;
    bool enabled;
    uint32_t sampleRate;
    float threshold;
    float lastConfidence;
    String lastKeyword;
    unsigned long inferenceTimeMs;

    KeywordCallback keywordCallback;

    // Edge Impulse inference buffer
    float* inferenceBuffer;
    size_t inferenceBufferSize;

    // Temporary buffer for sample conversion
    float* sampleBuffer;
    size_t sampleBufferSize;

    // Process inference and check for keywords
    bool runInference(const int16_t* samples, size_t sampleCount);

    // Convert int16_t samples to float
    void convertSamples(const int16_t* input, float* output, size_t count);
};

#endif // SPEECH_RECOGNIZER_H
