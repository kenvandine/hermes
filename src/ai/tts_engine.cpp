#include "tts_engine.h"
#include "config.h"
#include <WiFiClient.h>

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
                         piperServerHost.c_str(), piperServerPort);
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

void TTSEngine::setPiperServer(const String& host, uint16_t port) {
    piperServerHost = host;
    piperServerPort = port;
    Serial.printf("[TTS] Piper server set to: %s:%d\n", host.c_str(), port);
}

void TTSEngine::update() {
    // For async operations if needed
    // Currently TTS is synchronous (blocking)
}

bool TTSEngine::speakPiper(const String& text) {
    if (piperServerHost.isEmpty()) {
        errorMessage = "Piper server host not set";
        Serial.println("[TTS] ERROR: Piper server host not configured");
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
    // Piper responds with raw WAV audio when sent plain text

    Serial.printf("[TTS] Connecting to: %s:%d\n", piperServerHost.c_str(), piperServerPort);

    WiFiClient client;
    client.setTimeout(15);  // 15 second timeout for read operations

    unsigned long connectStart = millis();
    if (!client.connect(piperServerHost.c_str(), piperServerPort, 15000)) {  // 15 second connect timeout
        Serial.printf("[TTS] Connection failed after %lu ms\n", millis() - connectStart);
        errorMessage = "Connection failed";
        return false;
    }

    Serial.printf("[TTS] Connected in %lu ms\n", millis() - connectStart);

    speaking = true;
    status = STREAMING;

    // Send plain text - Piper responds with WAV
    Serial.printf("[TTS] Sending text: %s\n", text.substring(0, 50).c_str());
    client.println(text);
    client.flush();

    // Wait for response
    unsigned long timeout = millis() + 15000;
    while (client.connected() && !client.available() && millis() < timeout) {
        delay(10);
    }

    if (!client.available()) {
        Serial.println("[TTS] No response from server");
        errorMessage = "No response";
        client.stop();
        return false;
    }

    Serial.println("[TTS] Receiving audio data...");

    // Read first 64 bytes to analyze the response
    uint8_t headerBuf[64];
    size_t headerRead = 0;
    while (headerRead < 64 && (client.connected() || client.available()) && millis() < timeout) {
        if (client.available()) {
            headerBuf[headerRead++] = client.read();
        } else {
            delay(1);
        }
    }

    // Debug: dump first 64 bytes as hex
    Serial.print("[TTS] First 64 bytes: ");
    for (int i = 0; i < 64 && i < (int)headerRead; i++) {
        Serial.printf("%02X ", headerBuf[i]);
    }
    Serial.println();

    // Also show as ASCII where printable
    Serial.print("[TTS] ASCII: ");
    for (int i = 0; i < 64 && i < (int)headerRead; i++) {
        char c = headerBuf[i];
        Serial.print((c >= 32 && c < 127) ? c : '.');
    }
    Serial.println();

    // Parse WAV header
    if (headerRead < 44 || memcmp(headerBuf, "RIFF", 4) != 0) {
        Serial.printf("[TTS] Not a WAV file (first 4 bytes: %02X %02X %02X %02X)\n",
                      headerBuf[0], headerBuf[1], headerBuf[2], headerBuf[3]);
        errorMessage = "Invalid audio format";
        client.stop();
        return false;
    }

    // Standard WAV header layout (assuming no extra chunks)
    uint32_t sampleRate = *((uint32_t*)(headerBuf + 24));
    uint16_t bitsPerSample = *((uint16_t*)(headerBuf + 34));
    uint16_t numChannels = *((uint16_t*)(headerBuf + 22));
    uint32_t dataSize = *((uint32_t*)(headerBuf + 40));

    Serial.printf("[TTS] WAV: %d ch, %dHz, %d-bit, data=%d bytes\n",
                  numChannels, sampleRate, bitsPerSample, dataSize);

    // Check if 'data' chunk is at expected position
    if (memcmp(headerBuf + 36, "data", 4) != 0) {
        Serial.printf("[TTS] Warning: Expected 'data' at offset 36, got: %c%c%c%c\n",
                      headerBuf[36], headerBuf[37], headerBuf[38], headerBuf[39]);
    }

    // Audio data starts at offset 44 - we already read some into headerBuf
    size_t audioOffset = 44;
    size_t audioInHeader = headerRead - audioOffset;

    // Check if audio pipeline is available
    if (!audioPipeline) {
        Serial.println("[TTS] ERROR: No audio pipeline");
        client.stop();
        return false;
    }
    if (!audioPipeline->getAudio()) {
        Serial.println("[TTS] ERROR: No audio HAL");
        client.stop();
        return false;
    }

    HALAudio* audio = audioPipeline->getAudio();

    // Nabu Casa speaker runs at fixed 48kHz (XMOS I2S master)
    const uint32_t speakerRate = 48000;

    Serial.printf("[TTS] Speaker ready: %s\n", audio->isSpeakerReady() ? "yes" : "no");

    // Buffers for audio processing
    static const size_t inputBufSize = 512;
    static const size_t outputBufSize = 1200;  // ~512 * 2.17
    static uint8_t inputBuffer[inputBufSize];
    static int16_t resampleBuffer[outputBufSize];
    size_t totalAudioBytes = 0;
    size_t samplesWritten = 0;

    // Process any audio already in headerBuf (bytes 44-63)
    if (audioInHeader > 0 && bitsPerSample == 16) {
        int16_t* audioSamples = (int16_t*)(headerBuf + audioOffset);
        size_t inputCount = audioInHeader / 2;

        // Debug first samples from header
        Serial.printf("[TTS] First audio bytes from header: ");
        for (size_t i = 0; i < min((size_t)16, audioInHeader); i++) {
            Serial.printf("%02X ", headerBuf[audioOffset + i]);
        }
        Serial.println();

        int16_t minIn = 0, maxIn = 0;
        for (size_t i = 0; i < inputCount; i++) {
            if (audioSamples[i] < minIn) minIn = audioSamples[i];
            if (audioSamples[i] > maxIn) maxIn = audioSamples[i];
        }
        Serial.printf("[TTS] Header audio: %d samples [%d,%d]\n", inputCount, minIn, maxIn);

        // Resample and play
        size_t outIdx = 0;
        for (size_t i = 0; i < inputCount && outIdx < outputBufSize - 3; i++) {
            int16_t s = audioSamples[i];
            resampleBuffer[outIdx++] = s;
            resampleBuffer[outIdx++] = s;
            if ((i % 5) == 4) resampleBuffer[outIdx++] = s;
        }
        size_t written = audio->writeSpeaker(resampleBuffer, outIdx);
        samplesWritten += written;
        totalAudioBytes += audioInHeader;
    }

    // Stream remaining audio data
    while ((client.connected() || client.available()) && millis() < timeout) {
        size_t available = client.available();

        if (available > 0) {
            size_t toRead = min(available, inputBufSize);
            size_t bytesRead = client.readBytes(inputBuffer, toRead);

            if (bytesRead > 0 && bitsPerSample == 16) {
                int16_t* audioSamples = (int16_t*)inputBuffer;
                size_t inputCount = bytesRead / 2;

                // Debug first few chunks
                if (totalAudioBytes < 2000) {
                    int16_t minIn = 0, maxIn = 0;
                    for (size_t i = 0; i < inputCount; i++) {
                        if (audioSamples[i] < minIn) minIn = audioSamples[i];
                        if (audioSamples[i] > maxIn) maxIn = audioSamples[i];
                    }
                    Serial.printf("[TTS] Chunk @%d: %d samples [%d,%d]\n",
                                  totalAudioBytes, inputCount, minIn, maxIn);
                }

                // Resample 22050Hz -> 48000Hz (~2.177x)
                size_t outIdx = 0;
                for (size_t i = 0; i < inputCount && outIdx < outputBufSize - 3; i++) {
                    int16_t s = audioSamples[i];
                    resampleBuffer[outIdx++] = s;
                    resampleBuffer[outIdx++] = s;
                    if ((i % 5) == 4) resampleBuffer[outIdx++] = s;
                }

                size_t written = audio->writeSpeaker(resampleBuffer, outIdx);
                samplesWritten += written;
                totalAudioBytes += bytesRead;
            }

            // Check if we should stop
            if (!speaking || paused) break;

        } else if (client.connected()) {
            delay(1);
        }
    }

    // Properly close the connection
    client.flush();
    client.stop();

    Serial.printf("[TTS] Playback complete: %d audio bytes, %d samples written\n",
                  totalAudioBytes, samplesWritten);

    return totalAudioBytes > 0;
}

String TTSEngine::textToPhonemes(const String& text) {
    // Simple text normalization for eSpeak
    // Full implementation would use eSpeak's phoneme engine
    String phonemes = text;
    phonemes.toLowerCase();
    return phonemes;
}
