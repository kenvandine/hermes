#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include <functional>

/**
 * Application State
 *
 * Represents the current state of the intercom application
 */
enum class AppState {
    IDLE,          // No activity, waiting for wake word or MQTT command
    LISTENING,     // Wake word detected, listening for voice command
    CALLING,       // Outgoing call initiated, waiting for response
    RINGING,       // Incoming call, waiting for user to accept/reject
    ACTIVE_CALL,   // Call in progress
    HANGING_UP,    // Call ending, cleanup in progress
    ERROR          // Error state
};

/**
 * StateMachine
 *
 * Manages application state transitions with callbacks.
 * Ensures valid state transitions and notifies observers.
 */
class StateMachine {
public:
    // State change callback
    typedef std::function<void(AppState oldState, AppState newState)> StateChangeCallback;

    StateMachine();
    ~StateMachine();

    /**
     * Get current state
     */
    AppState getState() const;

    /**
     * Transition to new state
     * @param newState Target state
     * @return true if transition is valid and completed
     */
    bool setState(AppState newState);

    /**
     * Check if in specific state
     */
    bool isIdle() const;
    bool isListening() const;
    bool isCalling() const;
    bool isRinging() const;
    bool isInCall() const;
    bool isHangingUp() const;
    bool isError() const;

    /**
     * Register callback for state changes
     */
    void onStateChange(StateChangeCallback callback);

    /**
     * Get state as string (for debugging)
     */
    String getStateString() const;
    static String stateToString(AppState state);

    /**
     * Check if transition is valid
     */
    bool isValidTransition(AppState from, AppState to) const;

private:
    AppState currentState;
    StateChangeCallback stateChangeCallback;

    // Notify callback
    void notifyStateChange(AppState oldState, AppState newState);
};

#endif // STATE_MACHINE_H
