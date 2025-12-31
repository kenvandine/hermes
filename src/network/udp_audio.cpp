#include "udp_audio.h"
#include "config.h"

UdpAudio::UdpAudio()
    : destPort(0),
      localPort(0),
      txSequence(0),
      rxSequence(0),
      packetsSent(0),
      packetsReceived(0),
      packetsLost(0),
      bytesSent(0),
      bytesReceived(0),
      active(false) {
}

UdpAudio::~UdpAudio() {
    stop();
}

bool UdpAudio::begin(uint16_t port) {
    Serial.printf("[UdpAudio] Starting on port %d\n", port);

    if (udp.listen(port)) {
        localPort = port;
        active = true;

        // Register packet handler
        udp.onPacket([this](AsyncUDPPacket packet) {
            this->handlePacket(packet);
        });

        Serial.printf("[UdpAudio] Listening on port %d\n", port);
        return true;
    } else {
        Serial.printf("[UdpAudio] ERROR: Failed to listen on port %d\n", port);
        return false;
    }
}

void UdpAudio::stop() {
    if (active) {
        udp.close();
        active = false;
        Serial.println("[UdpAudio] Stopped");
    }
}

void UdpAudio::setDestination(const IPAddress& ip, uint16_t port) {
    destIp = ip;
    destPort = port;
    Serial.printf("[UdpAudio] Destination set to %s:%d\n", ip.toString().c_str(), port);
}

bool UdpAudio::sendAudio(uint32_t sessionId, const uint8_t* payload, size_t payloadSize) {
    if (!active || destPort == 0) {
        return false;
    }

    if (!payload || payloadSize == 0 || payloadSize > OPUS_MAX_PACKET_SIZE) {
        return false;
    }

    // Build packet header
    AudioPacketHeader header;
    header.magic = AUDIO_PACKET_MAGIC;
    header.sessionId = sessionId;
    header.sequence = txSequence++;
    header.payloadSize = payloadSize;
    header.codecType = CODEC_TYPE_OPUS;
    header.flags = 0;

    // Allocate packet buffer
    size_t totalSize = sizeof(AudioPacketHeader) + payloadSize;
    uint8_t* packet = (uint8_t*)malloc(totalSize);
    if (!packet) {
        Serial.println("[UdpAudio] ERROR: Failed to allocate packet buffer");
        return false;
    }

    // Copy header and payload
    memcpy(packet, &header, sizeof(AudioPacketHeader));
    memcpy(packet + sizeof(AudioPacketHeader), payload, payloadSize);

    // Send packet
    bool success = udp.writeTo(packet, totalSize, destIp, destPort) == totalSize;

    if (success) {
        packetsSent++;
        bytesSent += payloadSize;
    } else {
        Serial.println("[UdpAudio] ERROR: Failed to send packet");
    }

    free(packet);
    return success;
}

void UdpAudio::onAudioReceived(AudioReceivedCallback callback) {
    audioReceivedCallback = callback;
}

uint16_t UdpAudio::getLocalPort() const {
    return localPort;
}

void UdpAudio::resetStats() {
    packetsSent = 0;
    packetsReceived = 0;
    packetsLost = 0;
    bytesSent = 0;
    bytesReceived = 0;
    txSequence = 0;
    rxSequence = 0;
}

bool UdpAudio::isActive() const {
    return active;
}

void UdpAudio::handlePacket(AsyncUDPPacket& packet) {
    size_t packetSize = packet.length();

    // Validate minimum packet size
    if (packetSize < sizeof(AudioPacketHeader)) {
        Serial.println("[UdpAudio] WARNING: Packet too small, ignoring");
        return;
    }

    // Parse header
    const AudioPacketHeader* header = (const AudioPacketHeader*)packet.data();

    // Validate header
    if (!validateHeader(header, packetSize)) {
        return;
    }

    // Detect packet loss
    if (packetsReceived > 0) {
        uint32_t expectedSeq = rxSequence + 1;
        if (header->sequence != expectedSeq) {
            uint32_t lost = (header->sequence > expectedSeq) ?
                            (header->sequence - expectedSeq) : 0;
            if (lost > 0 && lost < 100) {  // Sanity check
                packetsLost += lost;
                Serial.printf("[UdpAudio] WARNING: Lost %d packet(s)\n", lost);
            }
        }
    }

    rxSequence = header->sequence;
    packetsReceived++;
    bytesReceived += header->payloadSize;

    // Extract payload
    const uint8_t* payload = packet.data() + sizeof(AudioPacketHeader);

    // Call callback with payload
    if (audioReceivedCallback) {
        audioReceivedCallback(payload, header->payloadSize, header->sequence);
    }
}

bool UdpAudio::validateHeader(const AudioPacketHeader* header, size_t packetSize) {
    // Check magic number
    if (header->magic != AUDIO_PACKET_MAGIC) {
        Serial.printf("[UdpAudio] WARNING: Invalid magic: 0x%08X\n", header->magic);
        return false;
    }

    // Check codec type
    if (header->codecType != CODEC_TYPE_OPUS) {
        Serial.printf("[UdpAudio] WARNING: Unsupported codec type: 0x%02X\n", header->codecType);
        return false;
    }

    // Check payload size
    size_t expectedSize = sizeof(AudioPacketHeader) + header->payloadSize;
    if (packetSize != expectedSize) {
        Serial.printf("[UdpAudio] WARNING: Size mismatch: expected %d, got %d\n",
                      expectedSize, packetSize);
        return false;
    }

    return true;
}
