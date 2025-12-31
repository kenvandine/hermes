#ifndef CALL_MANAGER_H
#define CALL_MANAGER_H

#include <Arduino.h>
#include "call_session.h"
#include "device_registry.h"
#include "../core/state_machine.h"

// Forward declarations
class MqttClient;
class AudioPipeline;

/**
 * CallManager
 *
 * Orchestrates the complete call lifecycle:
 * - Initiating outgoing calls
 * - Handling incoming calls
 * - Managing call state transitions
 * - Coordinating MQTT signaling and audio pipeline
 * - Timeout handling
 */
class CallManager {
public:
    CallManager(StateMachine* stateMachine,
                DeviceRegistry* deviceRegistry,
                MqttClient* mqttClient,
                AudioPipeline* audioPipeline);
    ~CallManager();

    /**
     * Initialize call manager
     */
    bool begin();

    /**
     * Process call state (call from main loop)
     * Handles timeouts and state transitions
     */
    void process();

    /**
     * Initiate outgoing call to room
     * @param roomName Target room name
     * @return true if call initiated successfully
     */
    bool initiateCall(const String& roomName);

    /**
     * Handle incoming call request
     * @param fromDeviceId Caller device ID
     * @param fromRoom Caller room name
     * @param sessionId Session identifier
     */
    void handleIncomingCall(const String& fromDeviceId,
                            const String& fromRoom,
                            uint32_t sessionId);

    /**
     * Handle call accepted by remote
     * @param sessionId Session identifier
     * @param remoteIp Remote device IP
     * @param remotePort Remote UDP port
     */
    void handleCallAccepted(uint32_t sessionId,
                            const IPAddress& remoteIp,
                            uint16_t remotePort);

    /**
     * Handle call rejected by remote
     * @param sessionId Session identifier
     * @param reason Rejection reason
     */
    void handleCallRejected(uint32_t sessionId, const String& reason);

    /**
     * Handle call hangup from remote
     * @param sessionId Session identifier
     */
    void handleCallHangup(uint32_t sessionId);

    /**
     * Accept incoming call (user action)
     */
    bool acceptCall();

    /**
     * Reject incoming call (user action)
     */
    bool rejectCall(const String& reason = "User declined");

    /**
     * Hang up active call (user action)
     */
    bool hangupCall();

    /**
     * Get current call session
     */
    CallSession* getCurrentSession();

    /**
     * Check if in call
     */
    bool isInCall() const;

    /**
     * Check if call in progress (any non-idle state)
     */
    bool isCallInProgress() const;

private:
    StateMachine* stateMachine;
    DeviceRegistry* deviceRegistry;
    MqttClient* mqttClient;
    AudioPipeline* audioPipeline;

    CallSession* currentSession;

    unsigned long callStateTime;  // Time of last state change

    // Generate unique session ID
    uint32_t generateSessionId();

    // Start audio for active call
    bool startAudio();

    // Stop audio
    void stopAudio();

    // Check for call timeout
    void checkTimeout();

    // Send MQTT call request
    bool sendCallRequest(const String& targetDeviceId, uint32_t sessionId);

    // Send MQTT call accept
    bool sendCallAccept(const String& targetDeviceId, uint32_t sessionId);

    // Send MQTT call reject
    bool sendCallReject(const String& targetDeviceId, uint32_t sessionId, const String& reason);

    // Send MQTT call hangup
    bool sendCallHangup(const String& targetDeviceId, uint32_t sessionId);

    // Update Home Assistant sensors
    void updateHASensors();

    // Get local UDP port for this device
    uint16_t getLocalUdpPort();
};

#endif // CALL_MANAGER_H
