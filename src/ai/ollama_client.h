#ifndef OLLAMA_CLIENT_H
#define OLLAMA_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

/**
 * Client for Ollama REST API
 *
 * Handles communication with Ollama server for AI-powered queries.
 * Supports both simple generation and chat-based conversations with context.
 */
class OllamaClient {
public:
    struct Query {
        String text;                // User query text
        bool includeContext;        // Include conversation history

        Query() : includeContext(false) {}
        Query(const String& t, bool ctx = false) : text(t), includeContext(ctx) {}
    };

    struct Response {
        String text;                // AI response text
        bool success;               // Request succeeded
        String error;               // Error message if failed
        uint32_t responseTime;      // Response time in milliseconds

        Response() : success(false), responseTime(0) {}
    };

    OllamaClient();
    ~OllamaClient();

    // Initialize client with server URL and port
    bool begin(const String& serverUrl, uint16_t port = 11434);

    // Send query to Ollama and get response
    Response query(const Query& q);

    // Configuration
    void setModel(const String& modelName);
    void setTimeout(uint32_t timeoutMs);
    void setMaxTokens(uint16_t maxTokens);
    void setTemperature(float temp);
    void setSystemPrompt(const String& prompt);

    // Context management
    void clearContext();
    size_t getContextSize() const;

    // Connection test
    bool testConnection();

private:
    struct Message {
        String role;        // "system", "user", or "assistant"
        String content;     // Message content

        Message(const String& r, const String& c) : role(r), content(c) {}
    };

    HTTPClient http;
    String serverUrl;
    uint16_t serverPort;
    String modelName;
    String systemPrompt;
    uint32_t timeout;
    uint16_t maxTokens;
    float temperature;

    std::vector<Message> conversationHistory;

    // Build JSON request for /api/chat endpoint
    String buildChatRequest(const String& userQuery);

    // Parse JSON response from Ollama
    bool parseResponse(const String& jsonResponse, Response& response);

    // HTTP helpers
    bool httpPost(const String& endpoint, const String& payload, String& response);
    bool httpGet(const String& endpoint, String& response);
};

#endif // OLLAMA_CLIENT_H
