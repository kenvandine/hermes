/**
 * Simple Serial Test
 * Just prints "Hello" repeatedly to verify USB serial is working
 */

#include <Arduino.h>

void setup() {
    // Initialize USB CDC serial
    Serial.begin(115200);

    // Wait for USB serial to be ready (important for ESP32-S3 USB CDC)
    delay(2000);

    Serial.println("\n\n=================================");
    Serial.println("  SERIAL TEST - HELLO WORLD");
    Serial.println("=================================");
    Serial.println("If you see this, USB serial is working!");
    Serial.println();
}

void loop() {
    static int count = 0;

    Serial.print("Loop ");
    Serial.print(count++);
    Serial.println(" - Hello from ESP32-S3!");

    delay(1000);
}
