#include "audio_pipeline.h"
#include "config.h"

AudioPipeline::AudioPipeline(HALAudio* audioHal)
    : audio(audioHal),
      codec(nullptr),
      udp(nullptr),
      wakeWord(nullptr),
      micBuffer(nullptr),
      speakerBuffer(nullptr),
      mode(AudioMode::IDLE),
      sampleRate(0),
      sessionId(0),
      frameSize(0),
      frameSizeMs(OPUS_FRAME_SIZE_MS),
      micFrame(nullptr),
      speakerFrame(nullptr),
      opusPacket(nullptr),
      initialized(false),
      wakeWordEnabled(false),
      wakeWordCallback(nullptr) {
}

AudioPipeline::~AudioPipeline() {
    stop();
}

bool AudioPipeline::begin(uint32_t sampleRateHz) {
    Serial.println("[AudioPipeline] Initializing...");

    sampleRate = sampleRateHz;
    frameSize = (sampleRate * frameSizeMs) / 1000;  // e.g., 320 samples for 20ms @ 16kHz

    Serial.printf("[AudioPipeline] Sample rate: %d Hz\n", sampleRate);
    Serial.printf("[AudioPipeline] Frame size: %d samples (%d ms)\n", frameSize, frameSizeMs);

    // Initialize audio HAL (codec + I2S)
    if (!audio) {
        Serial.println("[AudioPipeline] ERROR: No audio HAL provided");
        stop();
        return false;
    }

    if (!audio->begin(sampleRate)) {
        Serial.println("[AudioPipeline] ERROR: Failed to initialize audio HAL");
        stop();
        return false;
    }

    // Audio HAL handles microphone and speaker initialization internally

    // Create Opus codec
    codec = new OpusCodec();
    if (!codec->beginEncoder(sampleRate, 1, OPUS_BITRATE, OPUS_COMPLEXITY)) {
        Serial.println("[AudioPipeline] ERROR: Failed to initialize Opus encoder");
        stop();
        return false;
    }
    if (!codec->beginDecoder(sampleRate, 1)) {
        Serial.println("[AudioPipeline] ERROR: Failed to initialize Opus decoder");
        stop();
        return false;
    }

    // Create UDP transport
    udp = new UdpAudio();

    // Register UDP receive callback
    udp->onAudioReceived([this](const uint8_t* payload, size_t size, uint32_t sequence) {
        this->onAudioReceived(payload, size, sequence);
    });

    // Create audio buffers
    micBuffer = new AudioBuffer(AUDIO_BUFFER_SIZE);
    speakerBuffer = new AudioBuffer(AUDIO_BUFFER_SIZE);

    // Allocate frame buffers
    micFrame = (int16_t*)malloc(frameSize * sizeof(int16_t));
    speakerFrame = (int16_t*)malloc(frameSize * sizeof(int16_t));
    opusPacket = (uint8_t*)malloc(OPUS_MAX_PACKET_SIZE);

    if (!micFrame || !speakerFrame || !opusPacket) {
        Serial.println("[AudioPipeline] ERROR: Failed to allocate frame buffers");
        stop();
        return false;
    }

    // Create and initialize wake word detector
    wakeWord = new WakeWord();
    if (wakeWord->begin(sampleRate, WAKE_WORD_THRESHOLD)) {
        wakeWordEnabled = true;
        Serial.println("[AudioPipeline] Wake word detector initialized");
    } else {
        Serial.println("[AudioPipeline] Wake word detector not available (disabled in config or missing model)");
        wakeWordEnabled = false;
    }

    initialized = true;
    Serial.println("[AudioPipeline] Initialized successfully");

    return true;
}

void AudioPipeline::stop() {
    endCall();

    // Audio HAL is managed externally, we don't delete it
    // (it's owned by main.cpp)

    if (codec) {
        delete codec;
        codec = nullptr;
    }

    if (udp) {
        udp->stop();
        delete udp;
        udp = nullptr;
    }

    if (wakeWord) {
        wakeWord->stop();
        delete wakeWord;
        wakeWord = nullptr;
    }

    if (micBuffer) {
        delete micBuffer;
        micBuffer = nullptr;
    }

    if (speakerBuffer) {
        delete speakerBuffer;
        speakerBuffer = nullptr;
    }

    if (micFrame) {
        free(micFrame);
        micFrame = nullptr;
    }

    if (speakerFrame) {
        free(speakerFrame);
        speakerFrame = nullptr;
    }

    if (opusPacket) {
        free(opusPacket);
        opusPacket = nullptr;
    }

    initialized = false;
    wakeWordEnabled = false;
    wakeWordCallback = nullptr;
    mode = AudioMode::IDLE;
}

void AudioPipeline::setMode(AudioMode newMode) {
    if (newMode == mode) {
        return;
    }

    Serial.printf("[AudioPipeline] Mode change: %d -> %d\n", (int)mode, (int)newMode);

    // Clear buffers when changing modes
    if (micBuffer) micBuffer->clear();
    if (speakerBuffer) speakerBuffer->clear();

    mode = newMode;
}

AudioMode AudioPipeline::getMode() const {
    return mode;
}

void AudioPipeline::process() {
    if (!initialized) {
        return;
    }

    switch (mode) {
        case AudioMode::IDLE:
            processIdle();
            break;

        case AudioMode::LOOPBACK:
            processLoopback();
            break;

        case AudioMode::TRANSMIT:
            processTransmit();
            break;

        case AudioMode::RECEIVE:
            processReceive();
            break;

        case AudioMode::CALL:
            processCall();
            break;
    }
}

bool AudioPipeline::startCall(const IPAddress& remoteIp, uint16_t remotePort, uint16_t localPort, uint32_t sessId) {
    if (!initialized) {
        Serial.println("[AudioPipeline] ERROR: Not initialized");
        return false;
    }

    Serial.printf("[AudioPipeline] Starting call: %s:%d (local port %d, session %u)\n",
                  remoteIp.toString().c_str(), remotePort, localPort, sessId);

    sessionId = sessId;

    // Start UDP
    if (!udp->begin(localPort)) {
        Serial.println("[AudioPipeline] ERROR: Failed to start UDP");
        return false;
    }

    udp->setDestination(remoteIp, remotePort);
    udp->resetStats();

    // Clear buffers
    micBuffer->clear();
    speakerBuffer->clear();

    // Set mode to CALL
    setMode(AudioMode::CALL);

    Serial.println("[AudioPipeline] Call started successfully");
    return true;
}

void AudioPipeline::endCall() {
    if (mode == AudioMode::CALL || mode == AudioMode::TRANSMIT || mode == AudioMode::RECEIVE) {
        Serial.println("[AudioPipeline] Ending call");

        if (udp) {
            udp->stop();
        }

        setMode(AudioMode::IDLE);
    }
}

bool AudioPipeline::isInCall() const {
    return mode == AudioMode::CALL;
}

void AudioPipeline::setVolume(uint8_t volume) {
    if (audio) {
        audio->setVolume(volume);
    }
}

void AudioPipeline::setMicGain(uint8_t gainStep) {
    if (audio) {
        audio->setMicGain(gainStep);
    }
}

void AudioPipeline::mute(bool enabled) {
    if (audio) {
        audio->mute(enabled);
    }
}

uint8_t AudioPipeline::getMicrophoneLevel() {
    if (!micBuffer) return 0;

    int16_t peak = micBuffer->getPeakLevel();
    micBuffer->resetPeak();

    // Convert 0-32767 to 0-100
    return (uint8_t)((peak * 100) / 32767);
}

uint8_t AudioPipeline::getSpeakerLevel() {
    if (!speakerBuffer) return 0;

    int16_t peak = speakerBuffer->getPeakLevel();
    speakerBuffer->resetPeak();

    // Convert 0-32767 to 0-100
    return (uint8_t)((peak * 100) / 32767);
}

void AudioPipeline::getStats(uint32_t& txPackets, uint32_t& rxPackets, uint32_t& lostPackets) {
    if (udp) {
        txPackets = udp->getPacketsSent();
        rxPackets = udp->getPacketsReceived();
        lostPackets = udp->getPacketsLost();
    } else {
        txPackets = 0;
        rxPackets = 0;
        lostPackets = 0;
    }
}

// ============================================================================
// Mode Processing Functions
// ============================================================================

void AudioPipeline::processIdle() {
    // In idle mode, run wake word detection if enabled
    if (wakeWordEnabled && wakeWord && wakeWord->isEnabled()) {
        // Read from microphone
        size_t samplesRead = audio->readMicrophone(micFrame, frameSize);
        if (samplesRead > 0) {
            // Process audio through wake word detector
            wakeWord->process(micFrame, samplesRead);

            // Check if wake word was detected
            if (wakeWord->isDetected()) {
                Serial.printf("[AudioPipeline] Wake word detected! (confidence: %.2f)\n",
                              wakeWord->getLastConfidence());

                // Trigger callback if registered
                if (wakeWordCallback) {
                    wakeWordCallback();
                }

                // Reset detection state for next trigger
                wakeWord->reset();
            }
        }
    } else {
        // No wake word detection, just wait
        delay(10);
    }
}

void AudioPipeline::processLoopback() {
    // Simple loopback: Mic → Speaker (for testing)

    // Read from microphone
    size_t samplesRead = audio->readMicrophone(micFrame, frameSize);
    if (samplesRead > 0) {
        // Write directly to speaker
        audio->writeSpeaker(micFrame, samplesRead);
    }
}

void AudioPipeline::processTransmit() {
    // Transmit only: Mic → Opus → UDP

    // Read from microphone
    size_t samplesRead = audio->readMicrophone(micFrame, frameSize);
    if (samplesRead >= frameSize) {
        // Encode with Opus
        int encoded = codec->encode(micFrame, frameSize, opusPacket, OPUS_MAX_PACKET_SIZE);
        if (encoded > 0) {
            // Send via UDP
            udp->sendAudio(sessionId, opusPacket, encoded);
        }
    }
}

void AudioPipeline::processReceive() {
    // Receive only: UDP → Opus → Speaker
    // (Receiving happens in callback, just play from buffer)

    if (speakerBuffer->available() >= frameSize) {
        size_t samplesRead = speakerBuffer->read(speakerFrame, frameSize);
        if (samplesRead > 0) {
            audio->writeSpeaker(speakerFrame, samplesRead);
        }
    } else {
        // Not enough data, write silence
        memset(speakerFrame, 0, frameSize * sizeof(int16_t));
        audio->writeSpeaker(speakerFrame, frameSize);
    }
}

void AudioPipeline::processCall() {
    // Full duplex: Transmit + Receive

    // TRANSMIT PATH: Mic → Opus → UDP
    size_t samplesRead = audio->readMicrophone(micFrame, frameSize);
    if (samplesRead >= frameSize) {
        int encoded = codec->encode(micFrame, frameSize, opusPacket, OPUS_MAX_PACKET_SIZE);
        if (encoded > 0) {
            udp->sendAudio(sessionId, opusPacket, encoded);
        }
    }

    // RECEIVE PATH: Speaker buffer → Speaker
    if (speakerBuffer->available() >= frameSize) {
        size_t samplesToPlay = speakerBuffer->read(speakerFrame, frameSize);
        if (samplesToPlay > 0) {
            audio->writeSpeaker(speakerFrame, samplesToPlay);
        }
    } else {
        // Jitter buffer underrun - play silence
        memset(speakerFrame, 0, frameSize * sizeof(int16_t));
        audio->writeSpeaker(speakerFrame, frameSize);
    }
}

// ============================================================================
// Callbacks
// ============================================================================

void AudioPipeline::onAudioReceived(const uint8_t* payload, size_t size, uint32_t sequence) {
    // Decode Opus packet
    int decoded = codec->decode(payload, size, speakerFrame, frameSize, false);

    if (decoded > 0) {
        // Write to speaker buffer (jitter buffer)
        size_t written = speakerBuffer->write(speakerFrame, decoded);

        if (written < decoded) {
            // Buffer overflow - drop samples
            Serial.println("[AudioPipeline] WARNING: Speaker buffer overflow");
        }
    } else {
        Serial.printf("[AudioPipeline] WARNING: Opus decode failed: %d\n", decoded);

        // Use packet loss concealment (FEC)
        decoded = codec->decode(nullptr, 0, speakerFrame, frameSize, true);
        if (decoded > 0) {
            speakerBuffer->write(speakerFrame, decoded);
        }
    }
}

// ============================================================================
// Wake Word Detection
// ============================================================================

void AudioPipeline::enableWakeWord(bool enabled) {
    if (wakeWord) {
        wakeWord->enable(enabled);
        wakeWordEnabled = enabled && wakeWord->isReady();
        Serial.printf("[AudioPipeline] Wake word detection %s\n",
                      wakeWordEnabled ? "enabled" : "disabled");
    }
}

bool AudioPipeline::isWakeWordEnabled() const {
    return wakeWordEnabled && wakeWord && wakeWord->isEnabled();
}

void AudioPipeline::onWakeWordDetected(WakeWordCallback callback) {
    wakeWordCallback = callback;
}

void AudioPipeline::setWakeWordThreshold(float threshold) {
    if (wakeWord) {
        wakeWord->setThreshold(threshold);
    }
}
