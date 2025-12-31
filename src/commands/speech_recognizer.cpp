#include "speech_recognizer.h"
#include "config.h"

// TODO: Uncomment when Edge Impulse model is trained and exported
// Include your Edge Impulse model header here:
// #include <your_voice_commands_project_inferencing.h>

SpeechRecognizer::SpeechRecognizer()
    : initialized(false),
      enabled(true),
      sampleRate(16000),
      threshold(0.8f),
      lastConfidence(0.0f),
      lastKeyword(""),
      inferenceTimeMs(0),
      keywordCallback(nullptr),
      inferenceBuffer(nullptr),
      inferenceBufferSize(0),
      sampleBuffer(nullptr),
      sampleBufferSize(0) {
}

SpeechRecognizer::~SpeechRecognizer() {
    stop();
}

bool SpeechRecognizer::begin(uint32_t rate, float thresh) {
#if !VOICE_COMMANDS_ENABLED
    Serial.println("[SpeechRecognizer] Voice commands are disabled in config.h");
    Serial.println("[SpeechRecognizer] To enable:");
    Serial.println("[SpeechRecognizer]   1. Train keyword spotting model on Edge Impulse");
    Serial.println("[SpeechRecognizer]   2. Include command keywords + room names as classes");
    Serial.println("[SpeechRecognizer]   3. Export as Arduino library");
    Serial.println("[SpeechRecognizer]   4. Add to lib/ directory");
    Serial.println("[SpeechRecognizer]   5. Uncomment #include in speech_recognizer.cpp");
    Serial.println("[SpeechRecognizer]   6. Set VOICE_COMMANDS_ENABLED=true in config.h");
    return false;
#else
    Serial.println("[SpeechRecognizer] Initializing speech recognizer...");

    sampleRate = rate;
    threshold = thresh;

    // TODO: Initialize Edge Impulse model
    // Example (adjust for your model):
    /*
    ei_impulse_result_t result = { 0 };

    // Get model info
    ei_printf("Inferencing settings:\\n");
    ei_printf("\\tInterval: %.2f ms.\\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\\tFrame size: %d\\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\\tSample length: %d ms.\\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\\tNo. of classes: %d\\n", sizeof(ei_classifier_inferencing_categories) /
                                        sizeof(ei_classifier_inferencing_categories[0]));

    // Print class labels
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        ei_printf("    %s\\n", ei_classifier_inferencing_categories[ix]);
    }

    // Allocate inference buffer
    inferenceBufferSize = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    inferenceBuffer = (float*)malloc(inferenceBufferSize * sizeof(float));

    if (!inferenceBuffer) {
        Serial.println("[SpeechRecognizer] ERROR: Failed to allocate inference buffer");
        return false;
    }
    */

    // Allocate sample conversion buffer
    sampleBufferSize = 512;  // Adjust based on your model's requirements
    sampleBuffer = (float*)malloc(sampleBufferSize * sizeof(float));

    if (!sampleBuffer) {
        Serial.println("[SpeechRecognizer] ERROR: Failed to allocate sample buffer");
        return false;
    }

    initialized = true;
    Serial.printf("[SpeechRecognizer] Initialized (threshold: %.2f)\\n", threshold);

    return true;
#endif
}

void SpeechRecognizer::stop() {
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

bool SpeechRecognizer::process(const int16_t* samples, size_t sampleCount) {
    if (!initialized || !enabled || !samples) {
        return false;
    }

#if VOICE_COMMANDS_ENABLED
    // Run inference on audio samples
    return runInference(samples, sampleCount);
#else
    // Stub implementation for testing
    return false;
#endif
}

void SpeechRecognizer::setThreshold(float thresh) {
    threshold = constrain(thresh, 0.0f, 1.0f);
    Serial.printf("[SpeechRecognizer] Threshold set to: %.2f\\n", threshold);
}

float SpeechRecognizer::getLastConfidence() const {
    return lastConfidence;
}

String SpeechRecognizer::getLastKeyword() const {
    return lastKeyword;
}

unsigned long SpeechRecognizer::getInferenceTime() const {
    return inferenceTimeMs;
}

bool SpeechRecognizer::isReady() const {
    return initialized;
}

void SpeechRecognizer::enable(bool en) {
    enabled = en;
    Serial.printf("[SpeechRecognizer] %s\\n", enabled ? "Enabled" : "Disabled");
}

bool SpeechRecognizer::isEnabled() const {
    return enabled && initialized;
}

void SpeechRecognizer::onKeywordDetected(KeywordCallback callback) {
    keywordCallback = callback;
}

bool SpeechRecognizer::runInference(const int16_t* samples, size_t sampleCount) {
#if !VOICE_COMMANDS_ENABLED
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
        Serial.printf("[SpeechRecognizer] ERROR: Inference failed (%d)\\n", res);
        return false;
    }

    // Find highest confidence class (excluding "noise")
    float maxConfidence = 0.0f;
    String detectedKeyword = "";

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        const char* label = result.classification[ix].label;
        float confidence = result.classification[ix].value;

        // Skip "noise" class
        if (strcmp(label, "noise") == 0 || strcmp(label, "_unknown") == 0) {
            continue;
        }

        // Track highest confidence
        if (confidence > maxConfidence) {
            maxConfidence = confidence;
            detectedKeyword = String(label);
        }
    }

    // Check if detection exceeds threshold
    if (maxConfidence >= threshold) {
        lastConfidence = maxConfidence;
        lastKeyword = detectedKeyword;

        Serial.printf("[SpeechRecognizer] *** KEYWORD DETECTED: %s (%.2f) ***\\n",
                      detectedKeyword.c_str(), maxConfidence);

        // Trigger callback
        if (keywordCallback) {
            keywordCallback(detectedKeyword, maxConfidence);
        }

        return true;
    }
    */

    return false;
#endif
}

void SpeechRecognizer::convertSamples(const int16_t* input, float* output, size_t count) {
    for (size_t i = 0; i < count; i++) {
        // Convert int16_t (-32768 to 32767) to float (-1.0 to 1.0)
        output[i] = (float)input[i] / 32768.0f;
    }
}
