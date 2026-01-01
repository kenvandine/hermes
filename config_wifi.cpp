// =============================================================================
// HERMES Configuration Tool (Legacy)
// =============================================================================
// This is a legacy configuration tool that writes WiFi and MQTT credentials
// directly to the device's NVS (non-volatile storage).
//
// RECOMMENDED: Use the build script instead (see BUILD.md):
//   source .env && ./build.sh upload
//
// This tool is kept for compatibility but requires editing this file with
// your credentials before uploading, which is less secure than using
// environment variables with the build script.
//
// To use this tool:
// 1. Edit the credentials below (around line 23 and 41)
// 2. Upload with: pio run -e config_wifi --target upload
// 3. Then upload the main firmware: ./build.sh upload
// =============================================================================

#include <Arduino.h>
#include <Preferences.h>

Preferences preferences;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n========================================");
    Serial.println("  HERMES Configuration Tool");
    Serial.println("========================================\n");

    // Initialize preferences
    preferences.begin("hermes", false);

    // Don't clear everything - just update what we need
    // (clearing device_id causes first-boot behavior which skips MQTT config)

    // Write WiFi credentials
    // NOTE: Edit these values before uploading!
    Serial.println("\nWriting WiFi credentials...");
    preferences.putString("wifi_ssid", "your-wifi-ssid");
    preferences.putString("wifi_pass", "your-wifi-password");

    Serial.println("✓ WiFi SSID set to: your-wifi-ssid");
    Serial.println("✓ WiFi Password set: ********");

    // Write MQTT broker configuration with authentication
    // NOTE: Edit these values before uploading!
    Serial.println("\nWriting MQTT broker configuration...");

    // First remove old keys to ensure clean write
    preferences.remove("mqtt_broker");
    preferences.remove("mqtt_port");
    preferences.remove("mqtt_user");
    preferences.remove("mqtt_pass");
    delay(100);  // Give NVS time to commit

    // Now write new values
    preferences.putString("mqtt_broker", "your-mqtt-broker-ip");
    preferences.putUShort("mqtt_port", 1883);
    preferences.putString("mqtt_user", "homeassistant");
    preferences.putString("mqtt_pass", "your-mqtt-password");
    delay(100);  // Give NVS time to commit

    Serial.println("✓ MQTT Broker: your-mqtt-broker-ip");
    Serial.println("✓ MQTT Port: 1883");
    Serial.println("✓ MQTT Auth: username='homeassistant', password='your-mqtt-password'");

    // Verify
    String ssid = preferences.getString("wifi_ssid", "");
    String pass = preferences.getString("wifi_pass", "");
    String broker = preferences.getString("mqtt_broker", "");
    uint16_t port = preferences.getUShort("mqtt_port", 0);
    String mqtt_user = preferences.getString("mqtt_user", "NOTFOUND");
    String mqtt_pass = preferences.getString("mqtt_pass", "NOTFOUND");

    Serial.println("\nVerifying stored configuration...");
    Serial.print("  WiFi SSID: ");
    Serial.println(ssid);
    Serial.print("  WiFi Password length: ");
    Serial.println(pass.length());
    Serial.print("  MQTT Broker: ");
    Serial.println(broker);
    Serial.print("  MQTT Port: ");
    Serial.println(port);
    Serial.print("  MQTT User: '");
    Serial.print(mqtt_user);
    Serial.print("' (len=");
    Serial.print(mqtt_user.length());
    Serial.println(")");
    Serial.print("  MQTT Pass: '");
    Serial.print(mqtt_pass);
    Serial.print("' (len=");
    Serial.print(mqtt_pass.length());
    Serial.println(")");

    preferences.end();

    Serial.println("\n========================================");
    Serial.println("✓ Configuration complete!");
    Serial.println("  You can now upload the main firmware");
    Serial.println("========================================\n");
}

void loop() {
    // Nothing to do
    delay(1000);
}
