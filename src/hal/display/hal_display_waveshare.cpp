/**
 * Waveshare Display HAL Implementation
 *
 * Handles Waveshare ESP32-S3-Touch-AMOLED-1.8" display hardware initialization
 */

#include "hal/display/hal_display_waveshare.h"
#include "devices/waveshare/device_config.h"
#include "devices/waveshare/device_pins.h"
#include "ui/Arduino_SH8601.h"
#include <Wire.h>
#include <Arduino.h>

HALDisplayWaveshare::HALDisplayWaveshare()
    : gfx_(nullptr)
    , brightness_(255) {  // Maximum brightness for AMOLED

    // Initialize capabilities
    capabilities_.hasFullUI = true;
    capabilities_.hasLEDRing = false;
    capabilities_.hasTouchScreen = true;
    capabilities_.width = DEVICE_DISPLAY_WIDTH;
    capabilities_.height = DEVICE_DISPLAY_HEIGHT;
    capabilities_.ledCount = 0;
}

HALDisplayWaveshare::~HALDisplayWaveshare() {
    if (gfx_) {
        delete gfx_;
        gfx_ = nullptr;
    }
}

bool HALDisplayWaveshare::begin() {
    Serial.println("[HAL-Display-Waveshare] Initializing AMOLED display...");

    // Initialize I2C for touch controller and TCA9554 GPIO expander
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Serial.println("[HAL-Display-Waveshare] I2C bus initialized");

    // Scan I2C bus to find devices
    Serial.println("[HAL-Display-Waveshare] Scanning I2C bus...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("[HAL-Display-Waveshare]   Found I2C device at 0x%02X\n", addr);
        }
    }

    // Initialize TCA9554 GPIO expander for display power control
    Serial.println("[HAL-Display-Waveshare] Searching for TCA9554 GPIO expander...");
    uint8_t tca_addr = 0;
    uint8_t possible_addrs[] = {0x20, 0x24, 0x21, 0x22, 0x23, 0x25, 0x26, 0x27};

    for (int i = 0; i < 8; i++) {
        Wire.beginTransmission(possible_addrs[i]);
        if (Wire.endTransmission() == 0) {
            tca_addr = possible_addrs[i];
            Serial.printf("[HAL-Display-Waveshare]   Found TCA9554 at 0x%02X\n", tca_addr);
            break;
        }
    }

    if (tca_addr == 0) {
        Serial.println("[HAL-Display-Waveshare] ✗ TCA9554 not found!");
        Serial.println("[HAL-Display-Waveshare] Proceeding without GPIO expander...");
    } else {
        // Configure all TCA9554 pins as outputs
        Wire.beginTransmission(tca_addr);
        Wire.write(0x03);  // Configuration register
        Wire.write(0x00);  // All pins as outputs
        if (Wire.endTransmission() != 0) {
            Serial.println("[HAL-Display-Waveshare] ✗ Failed to configure TCA9554!");
        } else {
            Serial.println("[HAL-Display-Waveshare] ✓ TCA9554 configured");
        }

        // Enable display power (pins 0, 1, 2, 6 HIGH)
        Wire.beginTransmission(tca_addr);
        Wire.write(0x01);  // Output Port register
        Wire.write(0x47);  // Set pins 0,1,2,6 HIGH (0b01000111)
        if (Wire.endTransmission() != 0) {
            Serial.println("[HAL-Display-Waveshare] ✗ Failed to enable display power!");
        } else {
            Serial.println("[HAL-Display-Waveshare] ✓ Display power enabled");
        }
    }

    delay(200);  // Wait for display power to stabilize

    // Create QSPI bus
    Serial.println("[HAL-Display-Waveshare] Initializing QSPI bus...");
    Arduino_DataBus *bus = new Arduino_ESP32QSPI(
        PIN_DISPLAY_CS,    /* CS */
        PIN_DISPLAY_SCL,   /* SCK */
        PIN_DISPLAY_SDA0,  /* SDIO0 */
        PIN_DISPLAY_SDA1,  /* SDIO1 */
        PIN_DISPLAY_SDA2,  /* SDIO2 */
        PIN_DISPLAY_SDA3   /* SDIO3 */
    );

    // Create SH8601 AMOLED display driver
    Serial.println("[HAL-Display-Waveshare] Creating SH8601 display driver...");
    gfx_ = new Arduino_SH8601(
        bus,
        GFX_NOT_DEFINED,  /* RST - not used */
        DEVICE_DISPLAY_ROTATION,  /* rotation */
        false,  /* IPS */
        DEVICE_DISPLAY_WIDTH,
        DEVICE_DISPLAY_HEIGHT
    );

    if (!gfx_->begin()) {
        Serial.println("[HAL-Display-Waveshare] ✗ Failed to initialize display!");
        return false;
    }

    Serial.println("[HAL-Display-Waveshare] ✓ Display initialized");

    // Set brightness
    setBrightness(brightness_);

    // Color test
    Serial.println("[HAL-Display-Waveshare] Running color test...");
    gfx_->fillScreen(RED);
    delay(500);
    gfx_->fillScreen(GREEN);
    delay(500);
    gfx_->fillScreen(BLUE);
    delay(500);
    gfx_->fillScreen(BLACK);
    Serial.println("[HAL-Display-Waveshare] ✓ Color test complete");

    Serial.println("[HAL-Display-Waveshare] ✓ Display HAL initialized");
    return true;
}

void HALDisplayWaveshare::update() {
    // Display updates are handled by UIManager/LVGL
    // No direct HAL updates needed
}

void HALDisplayWaveshare::setBrightness(uint8_t brightness) {
    brightness_ = brightness;

    if (gfx_) {
        // SH8601 has Display_Brightness method
        ((Arduino_SH8601*)gfx_)->Display_Brightness(brightness);
    }
}

void HALDisplayWaveshare::showState(AppState state) {
    (void)state;
    // State display is handled by UIManager
}

void HALDisplayWaveshare::showMessage(const String& message, uint32_t durationMs) {
    (void)message;
    (void)durationMs;
    // Messages are handled by UIManager
}

void HALDisplayWaveshare::showError(const String& error) {
    (void)error;
    // Errors are handled by UIManager
}

DisplayCapabilities HALDisplayWaveshare::getCapabilities() const {
    return capabilities_;
}

void HALDisplayWaveshare::updateDeviceList() {
    // Device list updates are handled by UIManager
}

void HALDisplayWaveshare::updateCallInfo() {
    // Call info updates are handled by UIManager
}
