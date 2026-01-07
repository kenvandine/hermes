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
    // Piper Wyoming protocol - raw TCP with HTTP/0.9 style response
    // HTTPClient doesn't work because the server returns raw audio without HTTP headers

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

    // Wyoming/Piper protocol: just send raw text, receive raw audio
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

    // Read and parse WAV header
    uint8_t wavHeader[44];
    size_t headerRead = 0;
    timeout = millis() + 5000;
    while (headerRead < 44 && (client.connected() || client.available()) && millis() < timeout) {
        if (client.available()) {
            wavHeader[headerRead++] = client.read();
        } else {
            delay(1);
        }
    }

    // Verify WAV header
    if (headerRead < 44 || memcmp(wavHeader, "RIFF", 4) != 0) {
        Serial.printf("[TTS] Invalid WAV header (got %d bytes, first 4: %c%c%c%c)\n",
                      headerRead,
                      headerRead > 0 ? wavHeader[0] : '?',
                      headerRead > 1 ? wavHeader[1] : '?',
                      headerRead > 2 ? wavHeader[2] : '?',
                      headerRead > 3 ? wavHeader[3] : '?');
        errorMessage = "Invalid audio format";
        client.stop();
        return false;
    }

    // Extract WAV parameters
    uint32_t dataSize = *((uint32_t*)(wavHeader + 40));
    uint32_t sampleRate = *((uint32_t*)(wavHeader + 24));
    uint16_t bitsPerSample = *((uint16_t*)(wavHeader + 34));

    Serial.printf("[TTS] WAV: %dHz, %d-bit, %d bytes\n", sampleRate, bitsPerSample, dataSize);

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
    // getSampleRate() returns requested rate, not actual hardware rate
    const uint32_t speakerRate = 48000;

    // Calculate resampling ratio: how many input samples per output sample
    // For 22050->48000: ratio = 22050/48000 = 0.459 (upsample, need more outputs)
    float resampleRatio = (float)sampleRate / (float)speakerRate;
    Serial.printf("[TTS] Resampling from %dHz to %dHz (ratio %.3f)\n",
                  sampleRate, speakerRate, resampleRatio);

    Serial.printf("[TTS] Speaker ready: %s\n", audio->isSpeakerReady() ? "yes" : "no");

    // Use static buffers to avoid stack overflow in Audio Task
    // Input buffer small, output buffer larger for upsampling (22050->48000 = 2.17x)
    static const size_t inputBufSize = 256;
    static const size_t outputBufSize = 600;  // ~256 * 2.17
    static uint8_t buffer[inputBufSize];
    static int16_t resampleBuffer[outputBufSize];
    size_t totalRead = 0;
    size_t samplesWritten = 0;
    float resamplePos = 0.0f;  // Fractional position for resampling

    // Stream audio data to speaker
    while (client.connected() || client.available()) {
        size_t available = client.available();

        if (available) {
            size_t toRead = min(available, inputBufSize);
            size_t bytesRead = client.readBytes(buffer, toRead);

            if (bytesRead > 0) {
                totalRead += bytesRead;

                // Play through speaker via HAL
                if (bitsPerSample == 16) {
                    int16_t* audioSamples = (int16_t*)buffer;
                    size_t inputCount = bytesRead / 2;
                    size_t outputCount = 0;

                    // Resample from WAV rate to speaker rate
                    // WAV=22050Hz, Speaker=48000Hz -> upsample by ~2.17x
                    // Use simple sample duplication (nearest neighbor)
                    size_t outIdx = 0;
                    for (size_t i = 0; i < inputCount && outIdx < outputBufSize - 3; i++) {
                        int32_t sample = audioSamples[i] * 32;
                        if (sample > 32767) sample = 32767;
                        if (sample < -32768) sample = -32768;
                        int16_t s = (int16_t)sample;

                        // Output each sample ~2.17 times (alternate 2 and 2-2-3 pattern)
                        resampleBuffer[outIdx++] = s;
                        resampleBuffer[outIdx++] = s;
                        if ((i % 6) < 1) {  // Every 6th sample, add extra copy
                            resampleBuffer[outIdx++] = s;
                        }
                    }
                    outputCount = outIdx;

                    // Debug first chunk - show sample values
                    if (totalRead < 600) {
                        int16_t minSample = 0, maxSample = 0;
                        for (size_t i = 0; i < inputCount; i++) {
                            if (audioSamples[i] < minSample) minSample = audioSamples[i];
                            if (audioSamples[i] > maxSample) maxSample = audioSamples[i];
                        }
                        Serial.printf("[TTS] Chunk: %d in, %d out, samples range [%d, %d]\n",
                                      inputCount, outputCount, minSample, maxSample);
                    }

                    size_t written = audio->writeSpeaker(resampleBuffer, outputCount);
                    samplesWritten += written;
                }

                // Check if we should stop
                if (!speaking || paused) {
                    break;
                }
            }
        } else if (client.connected()) {
            delay(1);
        }
    }

    // Drain any remaining data
    while (client.available()) {
        client.read();
    }

    // Properly close the connection
    client.flush();
    client.stop();

    Serial.printf("[TTS] Playback complete: %d bytes, %d samples written\n", totalRead, samplesWritten);

    return true;
}

String TTSEngine::textToPhonemes(const String& text) {
    // Simple text normalization for eSpeak
    // Full implementation would use eSpeak's phoneme engine
    String phonemes = text;
    phonemes.toLowerCase();
    return phonemes;
}
