#ifndef TTS_ENGINE_H
#define TTS_ENGINE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include "../audio/audio_pipeline.h"

/**
 * Text-to-Speech Engine
 *
 * Supports multiple TTS backends:
 * - Piper TTS (neural, server-based, recommended)
 * - eSpeak (lightweight, local, robotic)
 * - Cloud TTS (Google/AWS, fallback)
 */
class TTSEngine {
public:
    enum Engine {
        PIPER,              // Neural TTS, runs on server
        ESPEAK,             // Lightweight, runs on ESP32
        CLOUD_FALLBACK      // Cloud-based (Google/AWS)
    };

    enum Status {
        IDLE,
        CONNECTING,
        STREAMING,
        ERROR
    };

    TTSEngine();
    ~TTSEngine();

    // Initialize with engine type and audio pipeline
    bool begin(Engine engine, AudioPipeline* audio);

    // Speak text (async, returns immediately)
    bool speak(const String& text);

    // Control playback
    void stop();
    void pause();
    void resume();

    // Status
    bool isSpeaking() const;
    Status getStatus() const;
    String getError() const;

    // Configuration
    void setVolume(uint8_t volume);      // 0-100
    void setSpeed(float speed);          // 0.5 - 2.0
    void setPitch(float pitch);          // 0.5 - 2.0
    void setVoice(const String& voice);  // Voice name/ID

    // Server configuration (for Piper)
    void setPiperServer(const String& host, uint16_t port);

    // Update (call in loop for async operations)
    void update();

private:
    Engine currentEngine;
    AudioPipeline* audioPipeline;
    Status status;
    String errorMessage;

    // Piper TTS settings
    String piperServerHost;
    uint16_t piperServerPort;
    String piperVoice;
    HTTPClient http;

    // Playback settings
    uint8_t volume;
    float speed;
    float pitch;
    bool speaking;
    bool paused;

    // Current TTS request
    String currentText;
    size_t currentPosition;

    // Engine-specific implementations
    bool speakPiper(const String& text);
    bool speakEspeak(const String& text);
    bool speakCloud(const String& text);

    // Stream audio from Piper server
    bool streamPiperAudio(const String& text);

    // Helper: Convert text to phonemes for espeak
    String textToPhonemes(const String& text);
};

#endif // TTS_ENGINE_H
