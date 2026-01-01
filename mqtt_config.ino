/*
 * HERMES MQTT Configuration Tool
 *
 * This sketch sets the MQTT broker configuration in NVS storage.
 * Upload this sketch once, let it run, then re-upload the main firmware.
 */

#include <Preferences.h>

Preferences prefs;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("  HERMES MQTT Configuration Tool");
    Serial.println("========================================\n");

    // Open NVS in read/write mode
    if (!prefs.begin("hermes", false)) {
        Serial.println("ERROR: Failed to open NVS storage!");
        return;
    }

    Serial.println("Setting MQTT configuration...\n");

    // Configure MQTT broker
    prefs.putString("mqtt_broker", "192.168.1.234");
    prefs.putUShort("mqtt_port", 1883);

    // For anonymous access, we don't set username/password
    // But let's clear them if they were set before
    prefs.putString("mqtt_user", "");
    prefs.putString("mqtt_pass", "");

    Serial.println("Configuration saved!");
    Serial.println("========================================");
    Serial.println("Current MQTT Settings:");
    Serial.println("========================================");
    Serial.printf("Broker:   %s\n", prefs.getString("mqtt_broker", "(not set)").c_str());
    Serial.printf("Port:     %d\n", prefs.getUShort("mqtt_port", 1883));
    Serial.printf("Username: %s\n", prefs.getString("mqtt_user", "(not set)").c_str());
    Serial.printf("Password: %s\n", prefs.getString("mqtt_pass", "").length() > 0 ? "***" : "(not set)");
    Serial.println("========================================\n");

    prefs.end();

    Serial.println("Configuration complete!");
    Serial.println("You can now re-upload the main HERMES firmware.");
    Serial.println("========================================\n");
}

void loop() {
    // Nothing to do
    delay(1000);
}
