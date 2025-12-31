#include "tts_engine.h"
#include "config.h"

TTSEngine::TTSEngine()
    : currentEngine(PIPER),
      audioPipeline(nullptr),
      status(IDLE),
      piperServerPort(10200),
      piperVoice("en_US-lessac-medium"),
      volume(70),
      speed(1.0),
      pitch(1.0),
      speaking(false),
      paused(false),
      currentPosition(0) {
}

TTSEngine::~TTSEngine() {
    stop();
    http.end();
}

bool TTSEngine::begin(Engine engine, AudioPipeline* audio) {
    if (!audio) {
        Serial.println("[TTS] ERROR: Null AudioPipeline");
        return false;
    }

    currentEngine = engine;
    audioPipeline = audio;

    Serial.printf("[TTS] Initialized with engine: %d\n", engine);

    switch (engine) {
        case PIPER:
            Serial.printf("[TTS] Piper server: %s:%d\n",
                         piperServerUrl.c_str(), piperServerPort);
            break;

        case ESPEAK:
            Serial.println("[TTS] eSpeak (local synthesis)");
            break;

        case CLOUD_FALLBACK:
            Serial.println("[TTS] Cloud TTS (fallback)");
            break;
    }

    return true;
}

bool TTSEngine::speak(const String& text) {
    if (text.isEmpty()) {
        Serial.println("[TTS] ERROR: Empty text");
        return false;
    }

    if (speaking && !paused) {
        Serial.println("[TTS] Already speaking, stopping current");
        stop();
    }

    currentText = text;
    currentPosition = 0;

    Serial.printf("[TTS] Speaking: %s\n", text.substring(0, 50).c_str());

    switch (currentEngine) {
        case PIPER:
            return speakPiper(text);

        case ESPEAK:
            return speakEspeak(text);

        case CLOUD_FALLBACK:
            return speakCloud(text);

        default:
            errorMessage = "Unknown TTS engine";
            return false;
    }
}

void TTSEngine::stop() {
    if (speaking) {
        Serial.println("[TTS] Stopping");
        speaking = false;
        paused = false;
        status = IDLE;
        http.end();
    }
}

void TTSEngine::pause() {
    if (speaking && !paused) {
        Serial.println("[TTS] Paused");
        paused = true;
    }
}

void TTSEngine::resume() {
    if (speaking && paused) {
        Serial.println("[TTS] Resumed");
        paused = false;
    }
}

bool TTSEngine::isSpeaking() const {
    return speaking;
}

TTSEngine::Status TTSEngine::getStatus() const {
    return status;
}

String TTSEngine::getError() const {
    return errorMessage;
}

void TTSEngine::setVolume(uint8_t vol) {
    volume = constrain(vol, 0, 100);
    if (audioPipeline) {
        audioPipeline->setVolume(volume);
    }
}

void TTSEngine::setSpeed(float spd) {
    speed = constrain(spd, 0.5f, 2.0f);
}

void TTSEngine::setPitch(float p) {
    pitch = constrain(p, 0.5f, 2.0f);
}

void TTSEngine::setVoice(const String& voice) {
    piperVoice = voice;
}

void TTSEngine::setPiperServer(const String& url, uint16_t port) {
    piperServerUrl = url;
    piperServerPort = port;
    Serial.printf("[TTS] Piper server set to: %s:%d\n", url.c_str(), port);
}

void TTSEngine::update() {
    // For async operations if needed
    // Currently TTS is synchronous (blocking)
}

bool TTSEngine::speakPiper(const String& text) {
    if (piperServerUrl.isEmpty()) {
        errorMessage = "Piper server URL not set";
        Serial.println("[TTS] ERROR: Piper server URL not configured");
        return false;
    }

    status = CONNECTING;

    // Stream audio from Piper server
    if (!streamPiperAudio(text)) {
        status = ERROR;
        return false;
    }

    status = IDLE;
    speaking = false;

    return true;
}

bool TTSEngine::speakEspeak(const String& text) {
    // TODO: Implement local eSpeak synthesis
    // This would require compiling eSpeak for ESP32
    // For now, return error
    errorMessage = "eSpeak not implemented yet";
    Serial.println("[TTS] ERROR: eSpeak not implemented");
    return false;
}

bool TTSEngine::speakCloud(const String& text) {
    // TODO: Implement cloud TTS (Google/AWS)
    // For now, return error
    errorMessage = "Cloud TTS not implemented yet";
    Serial.println("[TTS] ERROR: Cloud TTS not implemented");
    return false;
}

bool TTSEngine::streamPiperAudio(const String& text) {
    // Piper Wyoming protocol uses a simple JSON request format
    String url = piperServerUrl + ":" + String(piperServerPort) + "/api/tts";

    Serial.printf("[TTS] Connecting to: %s\n", url.c_str());

    http.begin(url);
    http.setTimeout(10000);
    http.addHeader("Content-Type", "application/json");

    // Build JSON request
    String jsonRequest = "{\"text\":\"" + text + "\",\"voice\":\"" + piperVoice + "\"}";

    speaking = true;
    status = STREAMING;

    int httpCode = http.POST(jsonRequest);

    if (httpCode != HTTP_CODE_OK && httpCode != 200) {
        Serial.printf("[TTS] HTTP error: %d\n", httpCode);
        errorMessage = "HTTP error: " + String(httpCode);
        http.end();
        return false;
    }

    // Get the audio stream
    WiFiClient* stream = http.getStreamPtr();
    int contentLength = http.getSize();

    Serial.printf("[TTS] Receiving audio: %d bytes\n", contentLength);

    // Buffer for audio chunks
    const size_t bufferSize = 512;
    uint8_t buffer[bufferSize];
    int bytesRead = 0;
    int totalRead = 0;

    // Skip WAV header (44 bytes)
    if (contentLength > 44) {
        stream->readBytes(buffer, 44);
        totalRead += 44;
    }

    // Stream audio data to speaker
    while (http.connected() && (totalRead < contentLength || contentLength == -1)) {
        size_t available = stream->available();

        if (available) {
            int toRead = min(available, bufferSize);
            bytesRead = stream->readBytes(buffer, toRead);

            if (bytesRead > 0) {
                totalRead += bytesRead;

                // TODO: Implement proper audio playback through speaker
                // Need to add a method to AudioPipeline or access I2S manager directly
                // For now, just consume the audio data
                //
                // Convert uint8_t to int16_t for AudioPipeline
                // int16_t* audioSamples = (int16_t*)buffer;
                // size_t sampleCount = bytesRead / 2;
                //
                // Play through speaker (blocking)
                // if (audioPipeline) {
                //     audioPipeline->playSpeaker(audioSamples, sampleCount);
                // }

                // Check if we should stop
                if (!speaking || paused) {
                    break;
                }
            }
        } else {
            delay(1);
        }
    }

    http.end();

    Serial.printf("[TTS] Playback complete: %d bytes\n", totalRead);

    return true;
}

String TTSEngine::textToPhonemes(const String& text) {
    // Simple text normalization for eSpeak
    // Full implementation would use eSpeak's phoneme engine
    String phonemes = text;
    phonemes.toLowerCase();
    return phonemes;
}
