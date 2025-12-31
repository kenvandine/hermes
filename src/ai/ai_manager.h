#ifndef AI_MANAGER_H
#define AI_MANAGER_H

#include <Arduino.h>
#include "ollama_client.h"
#include "tts_engine.h"
#include "../ui/ui_manager.h"
#include "../network/mqtt_client.h"
#include "../core/state_machine.h"

/**
 * AI Manager
 *
 * Orchestrates the complete AI assistant workflow:
 * 1. Receives user query
 * 2. Sends to Ollama server
 * 3. Delivers response via:
 *    - Text-to-speech (TTS)
 *    - Display on screen
 *    - MQTT to Home Assistant
 */
class AIManager {
public:
    enum ResponseMode {
        TTS_ONLY = 0x01,
        DISPLAY_ONLY = 0x02,
        MQTT_ONLY = 0x04,
        ALL = TTS_ONLY | DISPLAY_ONLY | MQTT_ONLY
    };

    enum State {
        IDLE,
        QUERYING,           // Waiting for Ollama response
        DELIVERING,         // Delivering response (TTS, UI, MQTT)
        ERROR_STATE
    };

    AIManager(OllamaClient* ollama, TTSEngine* tts,
              UIManager* ui, MqttClient* mqtt, StateMachine* sm);
    ~AIManager();

    // Initialize
    bool begin();

    // Process user query (async)
    void processQuery(const String& query, bool includeContext = false);

    // Update loop (call frequently)
    void update();

    // Cancel current operation
    void cancel();

    // Configuration
    void setResponseMode(uint8_t mode);
    uint8_t getResponseMode() const;

    // Status
    State getState() const;
    String getLastQuery() const;
    String getLastResponse() const;
    String getError() const;
    uint32_t getResponseTime() const;

    // Reset state
    void reset();

private:
    // Dependencies
    OllamaClient* ollamaClient;
    TTSEngine* ttsEngine;
    UIManager* uiManager;
    MqttClient* mqttClient;
    StateMachine* stateMachine;

    // State
    State state;
    uint8_t responseMode;
    String currentQuery;
    String currentResponse;
    String errorMessage;
    unsigned long queryStartTime;
    uint32_t responseTime;

    // Async operation tracking
    bool queryInProgress;
    bool ttsInProgress;

    // Internal workflow methods
    void startQuery(const String& query, bool includeContext);
    void handleQueryComplete(const OllamaClient::Response& response);
    void deliverResponse(const String& response);
    void publishToHA(const String& query, const String& response);
    void handleError(const String& error);

    // State management
    void setState(State newState);
    void updateStateMachine();
};

#endif // AI_MANAGER_H
