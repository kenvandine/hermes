#include "wake_word.h"
#include "config.h"

// Edge Impulse wake word model
#include <Hey_Hermes_-_Wake_Word_inferencing.h>

WakeWord::WakeWord()
    : initialized(false),
      enabled(true),
      sampleRate(16000),
      threshold(WAKE_WORD_THRESHOLD),
      lastConfidence(0.0f),
      lastDetectionTime(0),
      debounceMs(WAKE_WORD_DEBOUNCE_MS),
      inferenceTimeMs(0),
      inferenceBuffer(nullptr),
      inferenceBufferSize(0),
      sampleBuffer(nullptr),
      sampleBufferSize(0) {
}

WakeWord::~WakeWord() {
    stop();
}

bool WakeWord::begin(uint32_t rate, float thresh) {
#if !WAKE_WORD_ENABLED
    Serial.println("[WakeWord] Wake word detection is disabled in config.h");
    Serial.println("[WakeWord] To enable:");
    Serial.println("[WakeWord]   1. Train model on Edge Impulse");
    Serial.println("[WakeWord]   2. Export as Arduino library");
    Serial.println("[WakeWord]   3. Add to lib/ directory");
    Serial.println("[WakeWord]   4. Uncomment #include in wake_word.cpp");
    Serial.println("[WakeWord]   5. Set WAKE_WORD_ENABLED=true in config.h");
    return false;
#else
    Serial.println("[WakeWord] Initializing wake word detection...");

    sampleRate = rate;
    threshold = thresh;

    // Print model info
    Serial.printf("[WakeWord] Model: %s\n", EI_CLASSIFIER_PROJECT_NAME);
    Serial.printf("[WakeWord] Sample rate: %d Hz\n", EI_CLASSIFIER_FREQUENCY);
    Serial.printf("[WakeWord] Sample length: %d ms (%d samples)\n",
                  EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16, EI_CLASSIFIER_RAW_SAMPLE_COUNT);
    Serial.printf("[WakeWord] Classes: %d\n", EI_CLASSIFIER_LABEL_COUNT);
    Serial.printf("[WakeWord] Slice size: %d samples\n", EI_CLASSIFIER_SLICE_SIZE);

    // Verify sample rate matches
    if (rate != EI_CLASSIFIER_FREQUENCY) {
        Serial.printf("[WakeWord] WARNING: Sample rate mismatch (%d vs %d)\n",
                      rate, EI_CLASSIFIER_FREQUENCY);
    }

    // Allocate inference buffer for DSP input
    inferenceBufferSize = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    inferenceBuffer = (float*)malloc(inferenceBufferSize * sizeof(float));

    if (!inferenceBuffer) {
        Serial.println("[WakeWord] ERROR: Failed to allocate inference buffer");
        return false;
    }

    // Allocate sample conversion buffer (one slice at a time)
    sampleBufferSize = EI_CLASSIFIER_SLICE_SIZE;
    sampleBuffer = (float*)malloc(sampleBufferSize * sizeof(float));

    if (!sampleBuffer) {
        Serial.println("[WakeWord] ERROR: Failed to allocate sample buffer");
        free(inferenceBuffer);
        inferenceBuffer = nullptr;
        return false;
    }

    initialized = true;
    Serial.printf("[WakeWord] ✓ Initialized (threshold: %.2f)\n", threshold);

    return true;
#endif
}

void WakeWord::stop() {
    if (inferenceBuffer) {
        free(inferenceBuffer);
        inferenceBuffer = nullptr;
    }

    if (sampleBuffer) {
        free(sampleBuffer);
        sampleBuffer = nullptr;
    }

    initialized = false;
}

bool WakeWord::process(const int16_t* samples, size_t sampleCount) {
    if (!initialized || !enabled || !samples) {
        return false;
    }

#if WAKE_WORD_ENABLED
    // Run inference on audio samples
    return runInference(samples, sampleCount);
#else
    // Stub implementation for testing
    // In real implementation, this would call Edge Impulse inference
    return false;
#endif
}

bool WakeWord::isDetected() {
    if (!initialized || !enabled) {
        return false;
    }

    // Check debouncing
    unsigned long now = millis();
    if (now - lastDetectionTime < debounceMs) {
        return false;
    }

    // Check if detection occurred
    bool detected = (lastConfidence >= threshold);

    if (detected) {
        lastDetectionTime = now;
        Serial.printf("[WakeWord] *** WAKE WORD DETECTED *** (confidence: %.2f)\n", lastConfidence);
    }

    return detected;
}

void WakeWord::reset() {
    lastConfidence = 0.0f;
    lastDetectionTime = 0;
}

void WakeWord::setThreshold(float thresh) {
    threshold = constrain(thresh, 0.0f, 1.0f);
    Serial.printf("[WakeWord] Threshold set to: %.2f\n", threshold);
}

float WakeWord::getLastConfidence() const {
    return lastConfidence;
}

unsigned long WakeWord::getInferenceTime() const {
    return inferenceTimeMs;
}

bool WakeWord::isReady() const {
    return initialized;
}

void WakeWord::enable(bool en) {
    enabled = en;
    Serial.printf("[WakeWord] %s\n", enabled ? "Enabled" : "Disabled");
}

bool WakeWord::isEnabled() const {
    return enabled && initialized;
}

bool WakeWord::runInference(const int16_t* samples, size_t sampleCount) {
#if !WAKE_WORD_ENABLED
    return false;
#else
    unsigned long startTime = millis();

    // Edge Impulse expects exactly EI_CLASSIFIER_RAW_SAMPLE_COUNT samples (16000)
    if (sampleCount < EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        return false;  // Not enough samples yet
    }

    // Convert samples to float (-1.0 to 1.0 range)
    convertSamples(samples, inferenceBuffer, EI_CLASSIFIER_RAW_SAMPLE_COUNT);

    // Prepare signal structure for Edge Impulse
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = [](size_t offset, size_t length, float *out_ptr) -> int {
        // This is called by Edge Impulse to get chunks of data
        // We already have all data in inferenceBuffer, so just copy it
        return EIDSP_OK;
    };

    // Use numpy to create signal from our buffer
    numpy::signal_from_buffer(inferenceBuffer, EI_CLASSIFIER_RAW_SAMPLE_COUNT, &signal);

    // Run classifier
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    inferenceTimeMs = millis() - startTime;

    if (res != EI_IMPULSE_OK) {
        Serial.printf("[WakeWord] ERROR: Inference failed (%d)\n", res);
        return false;
    }

    // Find the "Hey Hermes" class and check confidence
    // Edge Impulse model will have labels like "Hey Hermes", "noise", "unknown"
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        const char* label = result.classification[ix].label;
        float confidence = result.classification[ix].value;

        // Check if this is the wake word class (case-insensitive contains "hermes")
        if (strstr(label, "Hermes") != nullptr || strstr(label, "hermes") != nullptr) {
            lastConfidence = confidence;

            #ifdef DEBUG_WAKE_WORD
            Serial.printf("[WakeWord] %s: %.2f\n", label, confidence);
            #endif

            if (confidence >= threshold) {
                Serial.printf("[WakeWord] ✓ Detected! Confidence: %.2f (threshold: %.2f)\n",
                              confidence, threshold);
                return true;
            }
        }
    }

    return false;
#endif
}

void WakeWord::convertSamples(const int16_t* input, float* output, size_t count) {
    for (size_t i = 0; i < count; i++) {
        // Convert int16_t (-32768 to 32767) to float (-1.0 to 1.0)
        output[i] = (float)input[i] / 32768.0f;
    }
}
