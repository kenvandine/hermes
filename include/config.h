#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

// I2S Microphone Pins (INMP441 or similar)
#define I2S_MIC_NUM             I2S_NUM_0
#define I2S_MIC_SCK_PIN         26    // Bit Clock (BCLK)
#define I2S_MIC_WS_PIN          25    // Word Select (LRCLK)
#define I2S_MIC_SD_PIN          33    // Serial Data (DOUT)

// I2S Speaker/Amplifier Pins (MAX98357A or similar)
#define I2S_SPK_NUM             I2S_NUM_1
#define I2S_SPK_SCK_PIN         14    // Bit Clock (BCLK)
#define I2S_SPK_WS_PIN          12    // Word Select (LRCLK)
#define I2S_SPK_SD_PIN          13    // Serial Data (DIN)

// SPI Display Pins (Waveshare 4.3" - adjust for your specific model)
// Note: Configure TFT_eSPI User_Setup.h for your exact display model
#define TFT_MISO                -1    // Not used for most displays
#define TFT_MOSI                11
#define TFT_SCLK                10
#define TFT_CS                  9
#define TFT_DC                  8
#define TFT_RST                 7
#define TFT_BL                  6     // Backlight control

// Touch Screen Pins (typically shared SPI bus with display)
#define TOUCH_CS                5
#define TOUCH_IRQ               4     // Touch interrupt (optional)

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
// UI CONFIGURATION
// ============================================================================

// Display resolution (adjust for your display)
#define DISPLAY_WIDTH           480
#define DISPLAY_HEIGHT          272

// Display rotation (0, 1, 2, 3 = 0°, 90°, 180°, 270°)
#define DISPLAY_ROTATION        1

// Backlight brightness (0-255)
#define BACKLIGHT_BRIGHTNESS    200

// UI update rate (Hz)
#define UI_UPDATE_RATE_HZ       30

// Touch calibration (adjust for your display)
#define TOUCH_CALIBRATION_ENABLED  true

// ============================================================================
// WAKE WORD & VOICE COMMAND CONFIGURATION
// ============================================================================

// Wake word detection
#define WAKE_WORD_ENABLED       false  // Set true when Edge Impulse model ready
#define WAKE_WORD_THRESHOLD     0.8    // 0.0 - 1.0, higher = stricter
#define WAKE_WORD_DEBOUNCE_MS   1000   // Minimum time between detections

// Command listening timeout
#define COMMAND_TIMEOUT_MS      5000   // 5 seconds to speak command after wake

// Inference interval
#define INFERENCE_INTERVAL_MS   100    // Run wake word inference every 100ms

// ============================================================================
// CALL MANAGEMENT CONFIGURATION
// ============================================================================

// Call states
enum class CallState {
    IDLE,          // No active call
    INITIATING,    // Outgoing call request sent
    RINGING,       // Incoming call, awaiting user action
    ACCEPTING,     // User accepted, establishing connection
    ACTIVE,        // Call in progress
    HANGING_UP,    // Terminating call
    ERROR          // Error state
};

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
#define NVS_KEY_DEVICE_ID       "device_id"
#define NVS_KEY_ROOM_NAME       "room_name"
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
// FEATURE FLAGS
// ============================================================================

#define FEATURE_WIFI_MANAGER        true   // WiFi captive portal for setup
#define FEATURE_OTA_UPDATES         true   // Over-the-air firmware updates
#define FEATURE_WEB_CONFIG          false  // Web-based configuration (future)
#define FEATURE_BLUETOOTH           false  // Bluetooth support (future)
#define FEATURE_RECORDING           false  // Call recording (future)

// ============================================================================
// VERSION INFORMATION
// ============================================================================

#define FIRMWARE_VERSION            "0.1.0"
#define BUILD_DATE                  __DATE__
#define BUILD_TIME                  __TIME__

#endif // CONFIG_H
