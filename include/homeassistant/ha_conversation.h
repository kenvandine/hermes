#ifndef HA_CONVERSATION_H
#define HA_CONVERSATION_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

/**
 * Home Assistant Conversation API Client
 *
 * Provides integration with Home Assistant's Conversation API for voice-based
 * device control. Routes device control commands to HA's assistant for execution.
 */
class HAConversation {
public:
    /**
     * Response structure for conversation API calls
     */
    struct Response {
        bool success;            // Whether the request succeeded
        String speech;           // Response text to speak back to user
        String responseType;     // "action_done", "error", etc.
        String conversationId;   // For maintaining conversation context
        String error;            // Error message if failed

        Response() : success(false) {}
    };

    HAConversation();
    ~HAConversation();

    /**
     * Initialize the conversation client
     * @param host Home Assistant host URL (e.g., "http://192.168.1.100:8123")
     * @param token Long-lived access token from HA
     * @return true if initialization succeeded
     */
    bool begin(const String& host, const String& token);

    /**
     * Send a text command to Home Assistant Conversation API
     * @param text The command text (e.g., "turn on kitchen light")
     * @param conversationId Optional conversation ID for context (empty for new conversation)
     * @return Response structure with result and speech text
     */
    Response process(const String& text, const String& conversationId = "");

    /**
     * Test connection to Home Assistant API
     * @return true if HA is reachable and token is valid
     */
    bool testConnection();

private:
    HTTPClient http_;
    String haHost_;
    String authToken_;
    bool initialized_;

    /**
     * Internal HTTP POST helper
     * @param endpoint API endpoint path
     * @param payload JSON payload string
     * @param response Output parameter for response body
     * @return true if HTTP request succeeded (200 OK)
     */
    bool httpPost(const String& endpoint, const String& payload, String& response);

    /**
     * Parse JSON response from HA Conversation API
     * @param json Response JSON string
     * @param result Output Response structure
     */
    void parseResponse(const String& json, Response& result);
};

#endif // HA_CONVERSATION_H
