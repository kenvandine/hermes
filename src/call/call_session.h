#ifndef CALL_SESSION_H
#define CALL_SESSION_H

#include <Arduino.h>
#include <IPAddress.h>

/**
 * Call Direction
 */
enum class CallDirection {
    NONE,
    OUTGOING,
    INCOMING
};

/**
 * Call State
 */
enum class CallState {
    IDLE,          // No active call
    INITIATING,    // Outgoing call, waiting for response
    RINGING,       // Incoming call, waiting for user action
    CONNECTING,    // Call accepted, establishing audio
    ACTIVE,        // Call in progress
    HANGING_UP,    // Terminating call
    FAILED         // Call failed
};

/**
 * CallSession
 *
 * Represents a single call session between two devices.
 * Tracks call state, remote device info, and timing.
 */
class CallSession {
public:
    CallSession();
    ~CallSession();

    /**
     * Start outgoing call
     */
    void startOutgoing(const String& remoteDeviceId,
                       const String& remoteRoom,
                       uint32_t sessionId);

    /**
     * Handle incoming call
     */
    void startIncoming(const String& remoteDeviceId,
                       const String& remoteRoom,
                       uint32_t sessionId);

    /**
     * Accept call (for incoming calls)
     */
    void accept(const IPAddress& remoteIp, uint16_t remotePort);

    /**
     * Reject call
     */
    void reject(const String& reason = "");

    /**
     * Activate call (audio started)
     */
    void activate(const IPAddress& remoteIp, uint16_t remotePort, uint16_t localPort);

    /**
     * Hang up call
     */
    void hangup();

    /**
     * Mark call as failed
     */
    void fail(const String& reason = "");

    /**
     * Reset session
     */
    void reset();

    /**
     * Get call state
     */
    CallState getState() const;

    /**
     * Get call direction
     */
    CallDirection getDirection() const;

    /**
     * Get session ID
     */
    uint32_t getSessionId() const;

    /**
     * Get remote device info
     */
    String getRemoteDeviceId() const;
    String getRemoteRoom() const;
    IPAddress getRemoteIp() const;
    uint16_t getRemotePort() const;
    uint16_t getLocalPort() const;

    /**
     * Get call duration (milliseconds)
     */
    unsigned long getDuration() const;

    /**
     * Check if active
     */
    bool isActive() const;

    /**
     * Check if in progress (any non-idle state)
     */
    bool isInProgress() const;

    /**
     * Get state as string
     */
    String getStateString() const;

    /**
     * Print session info
     */
    void printInfo() const;

private:
    CallState state;
    CallDirection direction;

    uint32_t sessionId;
    String remoteDeviceId;
    String remoteRoom;
    IPAddress remoteIp;
    uint16_t remotePort;
    uint16_t localPort;

    unsigned long startTime;
    unsigned long connectTime;
    unsigned long endTime;

    String failureReason;
};

#endif // CALL_SESSION_H
