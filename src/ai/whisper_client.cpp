#include "ai/whisper_client.h"
#include <ArduinoJson.h>

WhisperClient::WhisperClient()
    : serverPort_(9000),
      language_("en"),
      timeoutMs_(10000),
      initialized_(false) {
}

WhisperClient::~WhisperClient() {
    http_.end();
}

bool WhisperClient::begin(const String& host, uint16_t port) {
    if (host.isEmpty()) {
        Serial.println("[Whisper] Error: Host is empty");
        return false;
    }

    // Remove trailing slash from host if present
    serverHost_ = host;
    if (serverHost_.endsWith("/")) {
        serverHost_.remove(serverHost_.length() - 1);
    }

    serverPort_ = port;
    initialized_ = true;

    Serial.printf("[Whisper] Initialized with host: %s:%d\n",
                  serverHost_.c_str(), serverPort_);
    return true;
}

WhisperClient::Response WhisperClient::transcribe(const int16_t* audioData,
                                                   size_t sampleCount,
                                                   uint32_t sampleRate,
                                                   uint8_t channels) {
    Response result;
    uint32_t startTime = millis();

    if (!initialized_) {
        result.error = "Not initialized";
        Serial.println("[Whisper] Error: Client not initialized");
        return result;
    }

    if (!audioData || sampleCount == 0) {
        result.error = "Invalid audio data";
        Serial.println("[Whisper] Error: Invalid audio data");
        return result;
    }

    Serial.printf("[Whisper] Transcribing %d samples at %dHz (%d channels)...\n",
                  sampleCount, sampleRate, channels);

    // Convert PCM to WAV format
    uint8_t* wavBuffer = nullptr;
    size_t wavSize = createWavData(audioData, sampleCount, sampleRate, channels, &wavBuffer);

    if (!wavBuffer || wavSize == 0) {
        result.error = "Failed to create WAV data";
        Serial.println("[Whisper] Error: Failed to create WAV data");
        return result;
    }

    Serial.printf("[Whisper] Created WAV data: %d bytes\n", wavSize);

    // Send to Whisper server
    String endpoint = serverHost_ + ":" + String(serverPort_) + "/transcribe";
    String responseText;

    if (!httpPostMultipart(endpoint, wavBuffer, wavSize, responseText)) {
        result.error = "HTTP request failed";
        Serial.println("[Whisper] HTTP request failed");
        free(wavBuffer);
        return result;
    }

    free(wavBuffer);

    // Parse response
    parseResponse(responseText, result);

    result.durationMs = millis() - startTime;

    if (result.success) {
        Serial.printf("[Whisper] ✓ Transcription: \"%s\" (%.0fms)\n",
                      result.text.c_str(), result.durationMs);
    } else {
        Serial.printf("[Whisper] ✗ Error: %s\n", result.error.c_str());
    }

    return result;
}

bool WhisperClient::testConnection() {
    if (!initialized_) {
        Serial.println("[Whisper] Error: Client not initialized");
        return false;
    }

    // Simple test: GET /api/health or /
    String endpoint = serverHost_ + ":" + String(serverPort_);

    http_.begin(endpoint);
    http_.setTimeout(5000);  // 5 second timeout for test

    int httpCode = http_.GET();
    http_.end();

    if (httpCode > 0) {
        Serial.printf("[Whisper] Connection test: OK (HTTP %d)\n", httpCode);
        return true;
    } else {
        Serial.printf("[Whisper] Connection test failed: HTTP %d\n", httpCode);
        return false;
    }
}

void WhisperClient::setTimeout(uint32_t timeoutMs) {
    timeoutMs_ = timeoutMs;
}

void WhisperClient::setLanguage(const String& language) {
    language_ = language;
    Serial.printf("[Whisper] Language set to: %s\n", language_.c_str());
}

size_t WhisperClient::createWavData(const int16_t* audioData, size_t sampleCount,
                                    uint32_t sampleRate, uint8_t channels,
                                    uint8_t** wavBuffer) {
    // WAV file header structure
    const size_t dataSize = sampleCount * sizeof(int16_t);
    const size_t wavSize = 44 + dataSize;  // 44-byte header + audio data

    *wavBuffer = (uint8_t*)malloc(wavSize);
    if (!*wavBuffer) {
        Serial.println("[Whisper] Error: Failed to allocate WAV buffer");
        return 0;
    }

    uint8_t* p = *wavBuffer;

    // RIFF header
    memcpy(p, "RIFF", 4); p += 4;
    uint32_t chunkSize = wavSize - 8;
    memcpy(p, &chunkSize, 4); p += 4;
    memcpy(p, "WAVE", 4); p += 4;

    // fmt sub-chunk
    memcpy(p, "fmt ", 4); p += 4;
    uint32_t subchunk1Size = 16;
    memcpy(p, &subchunk1Size, 4); p += 4;
    uint16_t audioFormat = 1;  // PCM
    memcpy(p, &audioFormat, 2); p += 2;
    uint16_t numChannels = channels;  // Convert uint8_t to uint16_t for proper 2-byte copy
    memcpy(p, &numChannels, 2); p += 2;
    memcpy(p, &sampleRate, 4); p += 4;
    uint32_t byteRate = sampleRate * channels * sizeof(int16_t);
    memcpy(p, &byteRate, 4); p += 4;
    uint16_t blockAlign = channels * sizeof(int16_t);
    memcpy(p, &blockAlign, 2); p += 2;
    uint16_t bitsPerSample = 16;
    memcpy(p, &bitsPerSample, 2); p += 2;

    // data sub-chunk
    memcpy(p, "data", 4); p += 4;
    uint32_t subchunk2Size = dataSize;
    memcpy(p, &subchunk2Size, 4); p += 4;

    // Audio data
    memcpy(p, audioData, dataSize);

    return wavSize;
}

bool WhisperClient::httpPostMultipart(const String& endpoint, const uint8_t* audioData,
                                       size_t audioSize, String& response) {
    // Create multipart form data
    String boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

    // Build multipart body
    String bodyStart = "--" + boundary + "\r\n";
    bodyStart += "Content-Disposition: form-data; name=\"audio\"; filename=\"audio.wav\"\r\n";
    bodyStart += "Content-Type: audio/wav\r\n\r\n";

    String bodyEnd = "\r\n--" + boundary + "--\r\n";

    size_t totalSize = bodyStart.length() + audioSize + bodyEnd.length();

    // Allocate buffer for complete body
    uint8_t* body = (uint8_t*)malloc(totalSize);
    if (!body) {
        Serial.println("[Whisper] Failed to allocate multipart body");
        return false;
    }

    // Assemble body
    size_t offset = 0;
    memcpy(body + offset, bodyStart.c_str(), bodyStart.length());
    offset += bodyStart.length();
    memcpy(body + offset, audioData, audioSize);
    offset += audioSize;
    memcpy(body + offset, bodyEnd.c_str(), bodyEnd.length());

    http_.begin(endpoint);
    http_.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
    http_.setTimeout(timeoutMs_);

    int httpCode = http_.POST(body, totalSize);
    free(body);

    if (httpCode == 200) {
        response = http_.getString();
        http_.end();
        return true;
    } else {
        Serial.printf("[Whisper] HTTP POST failed: %d\n", httpCode);
        if (httpCode > 0) {
            String errorBody = http_.getString();
            Serial.printf("[Whisper] Error response: %s\n", errorBody.c_str());
        }
        http_.end();
        return false;
    }
}

void WhisperClient::parseResponse(const String& json, Response& result) {
    DynamicJsonDocument doc(4096);  // Increased from 1024 for larger responses
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        result.success = false;
        result.error = "JSON parse error: " + String(error.c_str());
        Serial.printf("[Whisper] JSON parse error: %s\n", error.c_str());
        return;
    }

    // Try different response formats
    // Format 1: {"text": "transcription"}
    if (doc.containsKey("text")) {
        result.text = doc["text"].as<String>();
    }
    // Format 2: {"transcription": "text"}
    else if (doc.containsKey("transcription")) {
        result.text = doc["transcription"].as<String>();
    }
    // Format 3: {"results": [{"transcript": "text"}]}
    else if (doc.containsKey("results") && doc["results"].is<JsonArray>()) {
        JsonArray results = doc["results"];
        if (results.size() > 0 && results[0].containsKey("transcript")) {
            result.text = results[0]["transcript"].as<String>();
        }
    }

    // Optional fields
    if (doc.containsKey("confidence")) {
        result.confidence = doc["confidence"].as<float>();
    }

    if (doc.containsKey("language")) {
        result.language = doc["language"].as<String>();
    }

    if (doc.containsKey("error")) {
        result.success = false;
        result.error = doc["error"].as<String>();
        return;
    }

    // Check if we got valid text
    if (!result.text.isEmpty()) {
        result.text.trim();
        result.success = true;
    } else {
        result.success = false;
        result.error = "No transcription in response";
        Serial.println("[Whisper] No transcription found in response");
        Serial.println("[Whisper] Response JSON:");
        serializeJsonPretty(doc, Serial);
        Serial.println();
    }
}
