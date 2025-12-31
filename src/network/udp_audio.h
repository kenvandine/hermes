#ifndef UDP_AUDIO_H
#define UDP_AUDIO_H

#include <Arduino.h>
#include <AsyncUDP.h>
#include <functional>

/**
 * Audio Packet Header
 *
 * Prepended to every audio packet for identification and sequencing
 */
struct AudioPacketHeader {
    uint32_t magic;           // 0x494E5443 ("INTC")
    uint32_t sessionId;       // Session identifier
    uint32_t sequence;        // Packet sequence number
    uint16_t payloadSize;     // Size of audio payload in bytes
    uint8_t  codecType;       // 0x01 = Opus
    uint8_t  flags;           // Control flags (unused for now)
} __attribute__((packed));

#define AUDIO_PACKET_MAGIC 0x494E5443  // "INTC"
#define CODEC_TYPE_OPUS    0x01

/**
 * UdpAudio
 *
 * Handles UDP transport for real-time audio streaming.
 *
 * Features:
 * - Packet framing with header
 * - Sequence number tracking
 * - Packet loss detection
 * - Asynchronous sending and receiving
 */
class UdpAudio {
public:
    // Callback for received audio data
    typedef std::function<void(const uint8_t* payload, size_t size, uint32_t sequence)> AudioReceivedCallback;

    UdpAudio();
    ~UdpAudio();

    /**
     * Start listening for audio packets on specified port
     * @param port UDP port to listen on
     * @return true if successful
     */
    bool begin(uint16_t port);

    /**
     * Stop listening
     */
    void stop();

    /**
     * Set destination for outgoing packets
     * @param ip Destination IP address
     * @param port Destination UDP port
     */
    void setDestination(const IPAddress& ip, uint16_t port);

    /**
     * Send audio packet
     * @param sessionId Session identifier
     * @param payload Audio data (Opus encoded)
     * @param payloadSize Size of payload in bytes
     * @return true if sent successfully
     */
    bool sendAudio(uint32_t sessionId, const uint8_t* payload, size_t payloadSize);

    /**
     * Register callback for received audio
     */
    void onAudioReceived(AudioReceivedCallback callback);

    /**
     * Get local port
     */
    uint16_t getLocalPort() const;

    /**
     * Get statistics
     */
    uint32_t getPacketsSent() const { return packetsSent; }
    uint32_t getPacketsReceived() const { return packetsReceived; }
    uint32_t getPacketsLost() const { return packetsLost; }
    uint32_t getBytesSent() const { return bytesSent; }
    uint32_t getBytesReceived() const { return bytesReceived; }

    /**
     * Reset statistics
     */
    void resetStats();

    /**
     * Check if UDP is active
     */
    bool isActive() const;

private:
    AsyncUDP udp;

    IPAddress destIp;
    uint16_t destPort;
    uint16_t localPort;

    uint32_t txSequence;  // Transmit sequence number
    uint32_t rxSequence;  // Last received sequence number

    // Statistics
    uint32_t packetsSent;
    uint32_t packetsReceived;
    uint32_t packetsLost;
    uint32_t bytesSent;
    uint32_t bytesReceived;

    AudioReceivedCallback audioReceivedCallback;

    bool active;

    // Handle incoming packet
    void handlePacket(AsyncUDPPacket& packet);

    // Validate packet header
    bool validateHeader(const AudioPacketHeader* header, size_t packetSize);
};

#endif // UDP_AUDIO_H
