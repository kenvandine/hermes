#include "wake_word.h"
#include "config.h"
#include <SPIFFS.h>

// Edge Impulse wake word model
#include <Hey_Hermes_-_Wake_Word_inferencing.h>

// Override Edge Impulse memory allocation functions to use PSRAM
// These replace the weak symbols in edge-impulse-sdk/porting/clib/ei_classifier_porting.cpp
void *ei_malloc(size_t size) {
    // Try PSRAM first for large allocations
    void *ptr = nullptr;
    if (size > 1024) {
        ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        if (ptr) {
            return ptr;
        }
    }

    // Fallback to internal RAM
    ptr = heap_caps_malloc(size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!ptr) {
        Serial.printf("[EI_ALLOC] ERROR: malloc failed for %d bytes\n", size);
    }
    return ptr;
}

void *ei_calloc(size_t nitems, size_t size) {
    size_t total = nitems * size;
    // Try PSRAM first for large allocations
    void *ptr = nullptr;
    if (total > 1024) {
        ptr = heap_caps_calloc(nitems, size, MALLOC_CAP_SPIRAM);
        if (ptr) {
            return ptr;
        }
    }

    // Fallback to internal RAM
    ptr = heap_caps_calloc(nitems, size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!ptr) {
        Serial.printf("[EI_ALLOC] ERROR: calloc failed for %d bytes\n", total);
    }
    return ptr;
}

void ei_free(void *ptr) {
    if (ptr) {
        heap_caps_free(ptr);
    }
}

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
      sampleBufferSize(0),
      rollingBuffer(nullptr),
      rollingBufferSize(0),
      rollingBufferPos(0) {
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

    // Allocate inference buffer for DSP input in PSRAM
    inferenceBufferSize = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    inferenceBuffer = (float*)heap_caps_malloc(inferenceBufferSize * sizeof(float), MALLOC_CAP_SPIRAM);

    if (!inferenceBuffer) {
        Serial.println("[WakeWord] ERROR: Failed to allocate inference buffer in PSRAM");
        return false;
    }

    // Allocate sample conversion buffer (one slice at a time) in PSRAM
    sampleBufferSize = EI_CLASSIFIER_SLICE_SIZE;
    sampleBuffer = (float*)heap_caps_malloc(sampleBufferSize * sizeof(float), MALLOC_CAP_SPIRAM);

    if (!sampleBuffer) {
        Serial.println("[WakeWord] ERROR: Failed to allocate sample buffer in PSRAM");
        heap_caps_free(inferenceBuffer);
        inferenceBuffer = nullptr;
        return false;
    }

    // Allocate rolling buffer to accumulate audio samples (16000 samples = 1 second @ 16kHz) in PSRAM
    rollingBufferSize = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    rollingBuffer = (int16_t*)heap_caps_malloc(rollingBufferSize * sizeof(int16_t), MALLOC_CAP_SPIRAM);

    if (!rollingBuffer) {
        Serial.println("[WakeWord] ERROR: Failed to allocate rolling buffer in PSRAM");
        heap_caps_free(inferenceBuffer);
        heap_caps_free(sampleBuffer);
        inferenceBuffer = nullptr;
        sampleBuffer = nullptr;
        return false;
    }

    // Initialize rolling buffer position
    rollingBufferPos = 0;
    memset(rollingBuffer, 0, rollingBufferSize * sizeof(int16_t));

    // Verify PSRAM allocation
    Serial.printf("[WakeWord] Rolling buffer allocated at: %p\n", (void*)rollingBuffer);
    Serial.printf("[WakeWord] Is in PSRAM: %s\n",
                  heap_caps_get_allocated_size(rollingBuffer) > 0 ? "yes" : "unknown");

    initialized = true;
    Serial.printf("[WakeWord] ✓ Initialized (threshold: %.2f, buffer: %d samples)\n",
                  threshold, rollingBufferSize);

    return true;
#endif
}

void WakeWord::stop() {
    if (inferenceBuffer) {
        heap_caps_free(inferenceBuffer);
        inferenceBuffer = nullptr;
    }

    if (sampleBuffer) {
        heap_caps_free(sampleBuffer);
        sampleBuffer = nullptr;
    }

    if (rollingBuffer) {
        heap_caps_free(rollingBuffer);
        rollingBuffer = nullptr;
    }

    initialized = false;
}

bool WakeWord::process(const int16_t* samples, size_t sampleCount) {
    if (!initialized || !enabled || !samples || !rollingBuffer) {
        return false;
    }

#if WAKE_WORD_ENABLED
    // Accumulate samples into rolling buffer
    for (size_t i = 0; i < sampleCount; i++) {
        rollingBuffer[rollingBufferPos] = samples[i];
        rollingBufferPos++;

        // When buffer is full, run inference
        if (rollingBufferPos >= rollingBufferSize) {
            #if DEBUG_WAKE_WORD
            Serial.printf("[WakeWord] Buffer full (%d samples), running inference...\n", rollingBufferSize);
            #endif

            // Run inference on the full buffer
            bool detected = runInference(rollingBuffer, rollingBufferSize);

            // Slide the buffer by half the window size to create overlap
            // This ensures we don't miss wake words that span buffer boundaries
            size_t slideAmount = rollingBufferSize / 2;
            memmove(rollingBuffer, rollingBuffer + slideAmount,
                    (rollingBufferSize - slideAmount) * sizeof(int16_t));
            rollingBufferPos = rollingBufferSize - slideAmount;

            if (detected) {
                return true;
            }
        }
    }

    return false;
#else
    // Stub implementation for testing
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
    if (sampleCount != EI_CLASSIFIER_RAW_SAMPLE_COUNT) {
        Serial.printf("[WakeWord] WARNING: Expected %d samples but got %d\n",
                      EI_CLASSIFIER_RAW_SAMPLE_COUNT, sampleCount);
        return false;
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
    #if DEBUG_WAKE_WORD
    Serial.printf("[WakeWord] Inference took %lums, checking %d classes:\n", inferenceTimeMs, EI_CLASSIFIER_LABEL_COUNT);
    #endif

    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        const char* label = result.classification[ix].label;
        float confidence = result.classification[ix].value;

        #if DEBUG_WAKE_WORD
        Serial.printf("[WakeWord]   %s: %.2f\n", label, confidence);
        #endif

        // Check if this is the wake word class (case-insensitive contains "hermes")
        if (strstr(label, "Hermes") != nullptr || strstr(label, "hermes") != nullptr) {
            lastConfidence = confidence;

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

bool WakeWord::saveBufferToWAV(const char* filename) {
    if (!initialized || !rollingBuffer) {
        Serial.println("[WakeWord] Cannot save: not initialized");
        return false;
    }

    // WAV file header structure
    struct WAVHeader {
        char riff[4] = {'R', 'I', 'F', 'F'};
        uint32_t fileSize;
        char wave[4] = {'W', 'A', 'V', 'E'};
        char fmt[4] = {'f', 'm', 't', ' '};
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1;  // PCM
        uint16_t numChannels = 1;   // Mono
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample = 16;
        char data[4] = {'d', 'a', 't', 'a'};
        uint32_t dataSize;
    };

    WAVHeader header;
    header.sampleRate = sampleRate;
    header.byteRate = sampleRate * 2;  // 16-bit mono
    header.blockAlign = 2;
    header.dataSize = rollingBufferPos * 2;  // Current buffer size in bytes
    header.fileSize = 36 + header.dataSize;

    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file) {
        Serial.printf("[WakeWord] Failed to open %s for writing\n", filename);
        return false;
    }

    // Write WAV header
    file.write((uint8_t*)&header, sizeof(header));

    // Write audio data
    file.write((uint8_t*)rollingBuffer, header.dataSize);

    file.close();

    Serial.printf("[WakeWord] ✓ Saved %d samples (%d bytes) to %s\n",
                  rollingBufferPos, header.dataSize, filename);
    return true;
}

const int16_t* WakeWord::getBuffer(size_t* size) const {
    if (size) {
        *size = rollingBufferPos;
    }
    return rollingBuffer;
}
