#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

// I2S Audio Pins - Waveshare ESP32-S3-Touch-AMOLED-1.8"
// ES8311 codec with built-in microphone and speaker
// Based on ESPHome config for ESP32-S3-Touch-AMOLED-1.75 (same pinout)
#define I2S_MIC_NUM             I2S_NUM_0
#define I2S_MIC_SCK_PIN         9     // Bit Clock (BCLK) - shared
#define I2S_MIC_WS_PIN          45    // Word Select (LRCLK) - shared
#define I2S_MIC_SD_PIN          10    // Serial Data IN (DIN) - microphone
#define I2S_MIC_MCLK_PIN        42    // Master Clock (MCLK) - shared

// I2S Speaker Pins (ES8311 DAC output)
#define I2S_SPK_NUM             I2S_NUM_1
#define I2S_SPK_SCK_PIN         9     // Bit Clock (BCLK) - shared
#define I2S_SPK_WS_PIN          45    // Word Select (LRCLK) - shared
#define I2S_SPK_SD_PIN          8     // Serial Data OUT (DOUT) - speaker
#define I2S_SPK_MCLK_PIN        42    // Master Clock (MCLK) - shared

// Waveshare ESP32-S3 1.8" AMOLED Display Pins (SH8601 controller)
// This is an integrated board - pins are fixed, not customizable
// Display uses QSPI interface
#define TFT_CS                  12    // Chip select
#define TFT_RST                 -1    // Reset (not used)
#define TFT_SDA0                4     // QSPI Data 0
#define TFT_SDA1                5     // QSPI Data 1
#define TFT_SDA2                6     // QSPI Data 2
#define TFT_SDA3                7     // QSPI Data 3
#define TFT_SCL                 11    // QSPI Clock

// Touch Screen Pins (FT3168 capacitive touch controller via I2C)
#define TOUCH_SDA               15    // I2C Data
#define TOUCH_SCL               14    // I2C Clock
#define TOUCH_INT               21    // Touch interrupt

// Optional: Physical buttons (if not using touch screen exclusively)
#define BUTTON_ACCEPT_PIN       -1    // -1 = not used
#define BUTTON_REJECT_PIN       -1
#define BUTTON_HANGUP_PIN       -1

// Status LED (optional, for debugging)
#define LED_STATUS_PIN          -1    // -1 = not used

// ============================================================================
// AUDIO CONFIGURATION
// ============================================================================

// Audio sample rate (Hz)
#define AUDIO_SAMPLE_RATE       16000  // 16kHz for voice (wake word, calls)
                                       // Can increase to 48000 for better quality

// Audio bit depth
#define AUDIO_BITS_PER_SAMPLE   16     // 16-bit audio

// I2S DMA buffer configuration
#define I2S_DMA_BUF_COUNT       8      // Number of DMA buffers
#define I2S_DMA_BUF_LEN         512    // Samples per buffer

// Opus codec settings
#define OPUS_FRAME_SIZE_MS      20     // 20ms frames (320 samples @ 16kHz)
#define OPUS_BITRATE            24000  // 24 kbps for voice
#define OPUS_COMPLEXITY         5      // 0-10, higher = better quality but more CPU
#define OPUS_MAX_PACKET_SIZE    1500   // Max UDP packet size

// Audio buffer sizes (samples)
#define AUDIO_BUFFER_SIZE       (AUDIO_SAMPLE_RATE * 2)  // 2 seconds
#define JITTER_BUFFER_SIZE      3200   // 200ms @ 16kHz

// Volume defaults (0-100)
#define DEFAULT_MIC_VOLUME      80
#define DEFAULT_SPEAKER_VOLUME  70

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// WiFi
#define WIFI_CONNECT_TIMEOUT_MS 20000  // 20 seconds
#define WIFI_RECONNECT_DELAY_MS 5000   // 5 seconds between reconnect attempts

// MQTT
#define MQTT_PORT               1883
#define MQTT_KEEPALIVE          60     // Keepalive interval in seconds
#define MQTT_QOS                1      // QoS level (0, 1, or 2)
#define MQTT_RECONNECT_DELAY_MS 5000   // Delay between reconnect attempts

// Device discovery and presence
#define DEVICE_ANNOUNCE_INTERVAL_MS    30000  // Announce presence every 30s
#define DEVICE_TIMEOUT_MS              90000  // Consider offline after 90s no heartbeat

// UDP audio streaming
#define UDP_AUDIO_PORT_BASE     5000   // Base port, each device gets unique port
#define UDP_BUFFER_SIZE         2048   // UDP receive buffer size

// ============================================================================
// UI CONFIGURATION - Waveshare ESP32-S3 1.8" AMOLED
// ============================================================================

// Display resolution (Waveshare ESP32-S3 1.8" AMOLED: 368x448 portrait)
#define DISPLAY_WIDTH           368
#define DISPLAY_HEIGHT          448

// Display rotation (0, 1, 2, 3 = 0°, 90°, 180°, 270°)
// 0 = Portrait (368x448), 1 = Landscape (448x368)
// 2 = Portrait inverted, 3 = Landscape inverted
#define DISPLAY_ROTATION        0     // Portrait orientation

// Backlight brightness (0-255) - AMOLED backlight control
#define BACKLIGHT_BRIGHTNESS    180   // Lower for AMOLED to save power

// UI update rate (Hz)
#define UI_UPDATE_RATE_HZ       30

// Touch configuration (CST816S capacitive touch)
#define TOUCH_I2C_ADDRESS       0x15  // CST816S I2C address
#define CST816_SLAVE_ADDRESS    0x15  // Same as TOUCH_I2C_ADDRESS
#define TOUCH_CALIBRATION_ENABLED  false  // Capacitive touch doesn't need calibration

// UI Layout Constants (optimized for 368x448 portrait display)
#define UI_MARGIN               10    // Screen margin
#define UI_BUTTON_HEIGHT        60    // Touch button height
#define UI_BUTTON_SPACING       15    // Space between buttons
#define UI_HEADER_HEIGHT        50    // Header bar height
#define UI_FOOTER_HEIGHT        80    // Footer area height
#define UI_ICON_SIZE            48    // Icon size for buttons
#define UI_FONT_SIZE_HEADER     24    // Header text size
#define UI_FONT_SIZE_NORMAL     18    // Normal text size
#define UI_FONT_SIZE_SMALL      14    // Small text size

// UI Colors (optimized for AMOLED - use true black to save power)
#define UI_COLOR_BACKGROUND     0x0000  // True black for AMOLED
#define UI_COLOR_PRIMARY        0x07E0  // Green
#define UI_COLOR_SECONDARY      0x7BEF  // Light gray
#define UI_COLOR_ACCENT         0xFD20  // Orange
#define UI_COLOR_TEXT           0xFFFF  // White
#define UI_COLOR_TEXT_DIM       0x8410  // Dim white
#define UI_COLOR_CALLING        0x07FF  // Cyan
#define UI_COLOR_RINGING        0xFFE0  // Yellow
#define UI_COLOR_ACTIVE         0x07E0  // Green
#define UI_COLOR_ERROR          0xF800  // Red

// ============================================================================
// WAKE WORD & VOICE COMMAND CONFIGURATION
// ============================================================================

// Wake word detection
#define WAKE_WORD_ENABLED       false  // Set true when Edge Impulse model ready
#define WAKE_WORD_THRESHOLD     0.8    // 0.0 - 1.0, higher = stricter
#define WAKE_WORD_DEBOUNCE_MS   1000   // Minimum time between detections

// Voice command recognition
#define VOICE_COMMANDS_ENABLED  false  // Set true when keyword spotting model ready
#define COMMAND_THRESHOLD       0.8    // 0.0 - 1.0, higher = stricter

// Command listening timeout
#define COMMAND_TIMEOUT_MS      5000   // 5 seconds to speak command after wake

// Inference interval
#define INFERENCE_INTERVAL_MS   100    // Run wake word inference every 100ms

// ============================================================================
// CALL MANAGEMENT CONFIGURATION
// ============================================================================

// Call timeouts
#define CALL_RING_TIMEOUT_MS    30000  // 30 seconds for recipient to answer
#define CALL_ACCEPT_TIMEOUT_MS  10000  // 10 seconds to establish audio connection
#define CALL_MAX_DURATION_MS    0      // 0 = unlimited

// ============================================================================
// HOME ASSISTANT CONFIGURATION
// ============================================================================

// MQTT Discovery
#define HA_DISCOVERY_ENABLED    true
#define HA_DISCOVERY_PREFIX     "homeassistant"

// mDNS for auto-discovery
#define MDNS_ENABLED            true
#define MDNS_SERVICE_NAME       "homeassistant"  // Look for HA on network

// ============================================================================
// STORAGE CONFIGURATION
// ============================================================================

// NVS (Non-Volatile Storage) namespace
#define NVS_NAMESPACE           "intercom"

// Configuration keys
#define NVS_KEY_DEVICE_ID       "living_room"
#define NVS_KEY_ROOM_NAME       "Living Room"
#define NVS_KEY_WIFI_SSID       "wifi_ssid"
#define NVS_KEY_WIFI_PASS       "wifi_pass"
#define NVS_KEY_MQTT_BROKER     "mqtt_broker"
#define NVS_KEY_MQTT_PORT       "mqtt_port"
#define NVS_KEY_MQTT_USER       "mqtt_user"
#define NVS_KEY_MQTT_PASS       "mqtt_pass"
#define NVS_KEY_VOLUME_MIC      "volume_mic"
#define NVS_KEY_VOLUME_SPK      "volume_spk"
#define NVS_KEY_BRIGHTNESS      "brightness"
#define NVS_KEY_WAKE_THRESHOLD  "wake_thresh"

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================

// FreeRTOS task priorities (0-24, higher = more priority)
#define TASK_PRIORITY_AUDIO_RX      5   // Highest: audio receive
#define TASK_PRIORITY_AUDIO_TX      5   // Highest: audio transmit
#define TASK_PRIORITY_WAKE_WORD     4   // High: wake word detection
#define TASK_PRIORITY_MQTT          3   // Medium: MQTT handling
#define TASK_PRIORITY_UI            2   // Medium-low: UI updates
#define TASK_PRIORITY_BACKGROUND    1   // Lowest: maintenance

// Task stack sizes (bytes)
#define TASK_STACK_AUDIO_RX         4096
#define TASK_STACK_AUDIO_TX         4096
#define TASK_STACK_WAKE_WORD        8192  // Needs more for ML inference
#define TASK_STACK_MQTT             4096
#define TASK_STACK_UI               8192  // LVGL needs more stack
#define TASK_STACK_BACKGROUND       2048

// Task core pinning (ESP32 has 2 cores: 0 and 1)
// Core 0 typically handles WiFi/BT, Core 1 for application
#define TASK_CORE_AUDIO             1
#define TASK_CORE_WAKE_WORD         1
#define TASK_CORE_MQTT              0
#define TASK_CORE_UI                1

// Watchdog timeout (milliseconds)
#define WATCHDOG_TIMEOUT_MS         10000

// Serial debug baud rate
#define SERIAL_BAUD_RATE            115200

// Logging level (0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose)
#define LOG_LEVEL                   4

// ============================================================================
// OLLAMA AI ASSISTANT CONFIGURATION
// ============================================================================

// Ollama server settings
#define OLLAMA_SERVER_URL       "http://192.168.1.21"  // Ollama LXD container IP
#define OLLAMA_SERVER_PORT      11434                  // Default Ollama port
#define OLLAMA_MODEL_NAME       "llama3.2:3b"          // Model (fast, 2GB)
#define OLLAMA_TIMEOUT_MS       10000                  // 10 second timeout
#define OLLAMA_MAX_TOKENS       200                    // Concise responses
#define OLLAMA_TEMPERATURE      0.7                    // Creativity (0.0-2.0)

// Piper TTS server settings
#define PIPER_SERVER_URL        "http://192.168.1.20"  // Piper LXD container IP
#define PIPER_SERVER_PORT       10200                  // Piper Wyoming protocol port
#define PIPER_VOICE             "en_GB-alan-medium"  // Voice model name

// Whisper speech recognition server (optional)
#define WHISPER_SERVER_URL      "http://192.168.1.52"  // Whisper LXD container IP
#define WHISPER_SERVER_PORT     9000                   // Whisper API port
#define WHISPER_ENABLED         false                  // Enable server-side STT

// AI system prompt (concise responses for voice)
#define AI_SYSTEM_PROMPT        "You are a helpful home assistant. Keep responses under 50 words."

// TTS engine selection
#define TTS_ENGINE              TTSEngine::PIPER       // PIPER, ESPEAK, or CLOUD_FALLBACK
#define TTS_SPEED               1.0                    // Speech rate (0.5-2.0)

// AI response delivery mode (bitfield)
// AIManager::TTS_ONLY | AIManager::DISPLAY_ONLY | AIManager::MQTT_ONLY
#define AI_RESPONSE_MODE_DEFAULT  7                    // All three (0x01 | 0x02 | 0x04)

// ============================================================================
// FEATURE FLAGS
// ============================================================================

#define FEATURE_WIFI_MANAGER        true   // WiFi captive portal for setup
#define FEATURE_OTA_UPDATES         true   // Over-the-air firmware updates
#define FEATURE_WEB_CONFIG          false  // Web-based configuration (future)
#define FEATURE_BLUETOOTH           false  // Bluetooth support (future)
#define FEATURE_RECORDING           false  // Call recording (future)
#define FEATURE_AI_ASSISTANT        true   // Ollama AI assistant integration

// ============================================================================
// VERSION INFORMATION
// ============================================================================

#define FIRMWARE_VERSION            "0.1.0"
#define BUILD_DATE                  __DATE__
#define BUILD_TIME                  __TIME__

#endif // CONFIG_H
