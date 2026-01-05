#ifndef WHISPER_CLIENT_H
#define WHISPER_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>

/**
 * Whisper Speech-to-Text Client
 *
 * Provides integration with a Whisper server for accurate speech recognition.
 * Sends audio data to the server and receives text transcription.
 *
 * Supports standard Whisper API endpoints (faster-whisper, Wyoming protocol, etc.)
 */
class WhisperClient {
public:
    /**
     * Response structure for transcription requests
     */
    struct Response {
        bool success;           // Whether transcription succeeded
        String text;            // Transcribed text
        float confidence;       // Confidence score (0.0-1.0, if available)
        String language;        // Detected language (if available)
        String error;           // Error message if failed
        uint32_t durationMs;    // Processing time in milliseconds

        Response() : success(false), confidence(0.0f), durationMs(0) {}
    };

    WhisperClient();
    ~WhisperClient();

    /**
     * Initialize the Whisper client
     * @param host Whisper server host URL (e.g., "http://192.168.1.100")
     * @param port Whisper server port (default 9000)
     * @return true if initialization succeeded
     */
    bool begin(const String& host, uint16_t port = 9000);

    /**
     * Transcribe audio to text
     * @param audioData PCM audio data (16-bit signed)
     * @param sampleCount Number of samples
     * @param sampleRate Sample rate in Hz (default 16000)
     * @param channels Number of audio channels (default 1 = mono)
     * @return Response structure with transcription result
     */
    Response transcribe(const int16_t* audioData, size_t sampleCount,
                       uint32_t sampleRate = 16000, uint8_t channels = 1);

    /**
     * Test connection to Whisper server
     * @return true if server is reachable
     */
    bool testConnection();

    /**
     * Set request timeout
     * @param timeoutMs Timeout in milliseconds (default 10000)
     */
    void setTimeout(uint32_t timeoutMs);

    /**
     * Set language for transcription
     * @param language Language code (e.g., "en", "es", "fr") or "auto"
     */
    void setLanguage(const String& language);

private:
    HTTPClient http_;
    String serverHost_;
    uint16_t serverPort_;
    String language_;
    uint32_t timeoutMs_;
    bool initialized_;

    /**
     * Convert PCM audio to WAV format
     * @param audioData PCM audio data
     * @param sampleCount Number of samples
     * @param sampleRate Sample rate in Hz
     * @param channels Number of channels
     * @param wavBuffer Output buffer for WAV data
     * @return Size of WAV data in bytes
     */
    size_t createWavData(const int16_t* audioData, size_t sampleCount,
                         uint32_t sampleRate, uint8_t channels,
                         uint8_t** wavBuffer);

    /**
     * Send HTTP POST request with audio data
     * @param endpoint API endpoint path
     * @param audioData WAV audio data
     * @param audioSize Size of audio data
     * @param response Output response string
     * @return true if request succeeded
     */
    bool httpPostAudio(const String& endpoint, const uint8_t* audioData,
                       size_t audioSize, String& response);

    /**
     * Parse JSON response from Whisper server
     * @param json Response JSON string
     * @param result Output Response structure
     */
    void parseResponse(const String& json, Response& result);
};

#endif // WHISPER_CLIENT_H
