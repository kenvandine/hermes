#include "audio_pipeline.h"
#include "config.h"

AudioPipeline::AudioPipeline()
    : i2s(nullptr),
      codec(nullptr),
      udp(nullptr),
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
      initialized(false) {
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

    // Create I2S manager
    i2s = new I2SManager();
    if (!i2s->beginMicrophone(sampleRate) || !i2s->beginSpeaker(sampleRate)) {
        Serial.println("[AudioPipeline] ERROR: Failed to initialize I2S");
        stop();
        return false;
    }

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

    initialized = true;
    Serial.println("[AudioPipeline] Initialized successfully");

    return true;
}

void AudioPipeline::stop() {
    endCall();

    if (i2s) {
        i2s->stopMicrophone();
        i2s->stopSpeaker();
        delete i2s;
        i2s = nullptr;
    }

    if (codec) {
        delete codec;
        codec = nullptr;
    }

    if (udp) {
        udp->stop();
        delete udp;
        udp = nullptr;
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
    if (i2s) {
        i2s->setVolume(volume);
    }
}

void AudioPipeline::mute(bool enabled) {
    if (i2s) {
        i2s->mute(enabled);
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
    // In idle mode, do nothing
    delay(10);
}

void AudioPipeline::processLoopback() {
    // Simple loopback: Mic → Speaker (for testing)

    // Read from microphone
    size_t samplesRead = i2s->readMicrophone(micFrame, frameSize);
    if (samplesRead > 0) {
        // Write directly to speaker
        i2s->writeSpeaker(micFrame, samplesRead);
    }
}

void AudioPipeline::processTransmit() {
    // Transmit only: Mic → Opus → UDP

    // Read from microphone
    size_t samplesRead = i2s->readMicrophone(micFrame, frameSize);
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
            i2s->writeSpeaker(speakerFrame, samplesRead);
        }
    } else {
        // Not enough data, write silence
        memset(speakerFrame, 0, frameSize * sizeof(int16_t));
        i2s->writeSpeaker(speakerFrame, frameSize);
    }
}

void AudioPipeline::processCall() {
    // Full duplex: Transmit + Receive

    // TRANSMIT PATH: Mic → Opus → UDP
    size_t samplesRead = i2s->readMicrophone(micFrame, frameSize);
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
            i2s->writeSpeaker(speakerFrame, samplesToPlay);
        }
    } else {
        // Jitter buffer underrun - play silence
        memset(speakerFrame, 0, frameSize * sizeof(int16_t));
        i2s->writeSpeaker(speakerFrame, frameSize);
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
