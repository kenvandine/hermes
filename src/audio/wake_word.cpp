#include "wake_word.h"
#include "config.h"

// TODO: Uncomment when Edge Impulse model is trained and exported
// Include your Edge Impulse model header here:
// #include <your_project_name_inferencing.h>

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

    // TODO: Initialize Edge Impulse model
    // Example (adjust for your model):
    /*
    ei_impulse_result_t result = { 0 };

    // Get model info
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
                                        sizeof(ei_classifier_inferencing_categories[0]));

    // Allocate inference buffer
    inferenceBufferSize = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    inferenceBuffer = (float*)malloc(inferenceBufferSize * sizeof(float));

    if (!inferenceBuffer) {
        Serial.println("[WakeWord] ERROR: Failed to allocate inference buffer");
        return false;
    }
    */

    // Allocate sample conversion buffer
    sampleBufferSize = 512;  // Adjust based on your model's requirements
    sampleBuffer = (float*)malloc(sampleBufferSize * sizeof(float));

    if (!sampleBuffer) {
        Serial.println("[WakeWord] ERROR: Failed to allocate sample buffer");
        return false;
    }

    initialized = true;
    Serial.printf("[WakeWord] Initialized (threshold: %.2f)\n", threshold);

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
    // TODO: Implement Edge Impulse inference
    // Example implementation:
    /*
    unsigned long startTime = millis();

    // Convert samples to float
    convertSamples(samples, sampleBuffer, min(sampleCount, sampleBufferSize));

    // Prepare signal for inference
    signal_t signal;
    numpy::signal_from_buffer(sampleBuffer, sampleCount, &signal);

    // Run inference
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    inferenceTimeMs = millis() - startTime;

    if (res != EI_IMPULSE_OK) {
        Serial.printf("[WakeWord] ERROR: Inference failed (%d)\n", res);
        return false;
    }

    // Check results
    // Assuming your model has classes like: "noise", "unknown", "hey_intercom"
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (strcmp(result.classification[ix].label, "hey_intercom") == 0) {
            lastConfidence = result.classification[ix].value;

            if (lastConfidence >= threshold) {
                return true;
            }
        }
    }
    */

    return false;
#endif
}

void WakeWord::convertSamples(const int16_t* input, float* output, size_t count) {
    for (size_t i = 0; i < count; i++) {
        // Convert int16_t (-32768 to 32767) to float (-1.0 to 1.0)
        output[i] = (float)input[i] / 32768.0f;
    }
}
