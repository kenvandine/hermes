#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <Arduino.h>
#include "config.h"
#include "hal/hal_audio.h"
#include "opus_codec.h"
#include "audio_buffer.h"
#include "wake_word.h"
#include "../network/udp_audio.h"
#include <functional>

/**
 * Audio Pipeline Mode
 */
enum class AudioMode {
    IDLE,         // Not processing audio (wake word detection active)
    LOOPBACK,     // Mic → Speaker (for testing)
    TRANSMIT,     // Mic → Opus → UDP (one-way send)
    RECEIVE,      // UDP → Opus → Speaker (one-way receive)
    CALL          // Full duplex: (Mic → Opus → UDP) + (UDP → Opus → Speaker)
};

/**
 * AudioPipeline
 *
 * Orchestrates the complete audio flow:
 * - Capture from microphone (I2S)
 * - Encode with Opus
 * - Send via UDP
 * - Receive via UDP
 * - Decode with Opus
 * - Playback through speaker (I2S)
 *
 * Features:
 * - Multiple modes (idle, loopback, call)
 * - Jitter buffer for smooth playback
 * - Frame-based processing
 * - Configurable sample rate and quality
 */
class AudioPipeline {
public:
    /**
     * Create audio pipeline with HAL audio interface
     * @param audio HAL audio interface (must remain valid for lifetime of AudioPipeline)
     */
    AudioPipeline(HALAudio* audio);
    ~AudioPipeline();

    /**
     * Initialize audio pipeline
     * @param sampleRate Sample rate in Hz (16000 recommended)
     * @return true if successful
     */
    bool begin(uint32_t sampleRate = AUDIO_SAMPLE_RATE);

    /**
     * Stop audio pipeline
     */
    void stop();

    /**
     * Set audio mode
     */
    void setMode(AudioMode mode);

    /**
     * Get current mode
     */
    AudioMode getMode() const;

    /**
     * Process audio (call from audio task loop)
     * This handles all audio I/O, encoding, decoding
     */
    void process();

    /**
     * Start call with remote device
     * @param remoteIp Remote device IP
     * @param remotePort Remote UDP port
     * @param localPort Local UDP port to listen on
     * @param sessionId Session identifier
     */
    bool startCall(const IPAddress& remoteIp, uint16_t remotePort, uint16_t localPort, uint32_t sessionId);

    /**
     * End current call
     */
    void endCall();

    /**
     * Check if in call
     */
    bool isInCall() const;

    /**
     * Set speaker volume (0-100)
     */
    void setVolume(uint8_t volume);

    /**
     * Set microphone gain (0-6: 0dB to 36dB in 6dB steps)
     */
    void setMicGain(uint8_t gainStep);

    /**
     * Mute/unmute speaker
     */
    void mute(bool enabled);

    /**
     * Get audio level (0-100) from microphone
     */
    uint8_t getMicrophoneLevel();

    /**
     * Get audio level (0-100) from speaker
     */
    uint8_t getSpeakerLevel();

    /**
     * Get audio statistics
     */
    void getStats(uint32_t& txPackets, uint32_t& rxPackets, uint32_t& lostPackets);

    /**
     * Get components (for advanced use)
     */
    HALAudio* getAudio() { return audio; }
    OpusCodec* getCodec() { return codec; }
    UdpAudio* getUdp() { return udp; }
    WakeWord* getWakeWord() { return wakeWord; }

    /**
     * Wake word detection
     */
    typedef std::function<void()> WakeWordCallback;

    /**
     * Enable/disable wake word detection
     */
    void enableWakeWord(bool enabled);

    /**
     * Check if wake word detection is enabled
     */
    bool isWakeWordEnabled() const;

    /**
     * Register callback for wake word detection
     */
    void onWakeWordDetected(WakeWordCallback callback);

    /**
     * Set wake word detection threshold (0.0-1.0)
     */
    void setWakeWordThreshold(float threshold);

private:
    HALAudio* audio;  // Hardware abstraction layer for audio I/O
    OpusCodec* codec;
    UdpAudio* udp;
    WakeWord* wakeWord;

    AudioBuffer* micBuffer;      // Mic input buffer
    AudioBuffer* speakerBuffer;  // Speaker output buffer (jitter buffer)

    AudioMode mode;
    uint32_t sampleRate;
    uint32_t sessionId;

    int frameSize;      // Samples per frame (e.g., 320 for 20ms @ 16kHz)
    int frameSizeMs;    // Frame duration in milliseconds

    int16_t* micFrame;      // Temp buffer for microphone frame
    int16_t* speakerFrame;  // Temp buffer for speaker frame
    uint8_t* opusPacket;    // Temp buffer for Opus packet

    bool initialized;
    bool wakeWordEnabled;
    WakeWordCallback wakeWordCallback;

    // Processing functions for each mode
    void processIdle();
    void processLoopback();
    void processTransmit();
    void processReceive();
    void processCall();

    // Audio receive callback
    void onAudioReceived(const uint8_t* payload, size_t size, uint32_t sequence);
};

#endif // AUDIO_PIPELINE_H
