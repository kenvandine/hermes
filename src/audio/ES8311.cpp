/**
 * ES8311 Audio Codec Driver Implementation
 */

#include "ES8311.h"

ES8311::ES8311() : sample_rate_(16000) {
}

bool ES8311::begin(int i2c_sda, int i2c_scl, uint32_t sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize I2C
    Wire.begin(i2c_sda, i2c_scl);
    Wire.setClock(100000);  // 100kHz I2C clock

    delay(10);

    // Reset codec
    Serial.println("[ES8311] Resetting codec...");
    writeReg(ES8311_REG00, 0x1F);  // Reset and power down all modules
    delay(10);
    writeReg(ES8311_REG00, 0x00);  // Release reset
    delay(10);

    // Verify communication by reading chip ID
    uint8_t chipId = readReg(ES8311_REG0D);
    Serial.printf("[ES8311] Chip ID: 0x%02X\n", chipId);

    // Configure clocks
    Serial.println("[ES8311] Configuring clocks...");
    configureClocks(sample_rate);

    // Power up analog blocks
    Serial.println("[ES8311] Powering up analog blocks...");
    writeReg(ES8311_REG0D, 0x01);  // Power up analog
    writeReg(ES8311_REG0E, 0x02);  // Enable analog PGA
    writeReg(ES8311_REG12, 0x00);  // Enable ALC
    writeReg(ES8311_REG13, 0x10);  // ALC settings
    delay(10);

    // Configure ADC (microphone)
    Serial.println("[ES8311] Configuring ADC...");
    configureADC();

    // Configure DAC (speaker)
    Serial.println("[ES8311] Configuring DAC...");
    configureDAC();

    // Set default gain and volume
    setMicGain(GAIN_36DB);      // 36dB microphone gain
    setVolume(70);              // 70% speaker volume
    muteMic(false);             // Unmute microphone
    muteDAC(false);             // Unmute speaker

    // Verify critical ADC registers
    Serial.println("[ES8311] Verifying ADC configuration:");
    Serial.printf("  REG02 (CLK_SRC): 0x%02X (expect 0x00 for MCLK)\n", readReg(ES8311_REG02));
    Serial.printf("  REG14 (MIC_BIAS): 0x%02X\n", readReg(ES8311_REG14));
    Serial.printf("  REG16 (ADC_SCALE): 0x%02X\n", readReg(ES8311_REG16));
    Serial.printf("  REG17 (ADC_PDN): 0x%02X (expect 0xBF for powered up)\n", readReg(ES8311_REG17));
    Serial.printf("  REG15 (ADC_MUTE): 0x%02X (expect 0x40 for unmuted)\n", readReg(ES8311_REG15));
    Serial.printf("  REG0E (PGA_GAIN): 0x%02X\n", readReg(ES8311_REG0E));

    Serial.println("[ES8311] ✓ Codec initialized");
    return true;
}

void ES8311::configureClocks(uint32_t sample_rate) {
    // Clock configuration using MCLK (Master Clock)
    // MCLK is provided by I2S peripheral on GPIO 42

    writeReg(ES8311_REG01, 0x3F);  // Clock manager: CLK_ON, enable all clocks
    writeReg(ES8311_REG02, 0x00);  // Use MCLK as clock source (from MCLK pin)
    writeReg(ES8311_REG03, 0x10);  // ADC fsmode and osr
    writeReg(ES8311_REG04, 0x10);  // DAC osr
    writeReg(ES8311_REG05, 0x00);  // Clock divider for 16kHz

    // Additional clock settings
    writeReg(ES8311_REG06, 0x00);  // Normal mode
    writeReg(ES8311_REG07, 0x00);  // Additional clock config
    writeReg(ES8311_REG08, 0x00);  // Clock inversion control
}

void ES8311::configureADC() {
    // ADC configuration based on ESP-ADF reference

    // System configuration
    writeReg(ES8311_REG0B, 0x00);  // System control
    writeReg(ES8311_REG0C, 0x00);  // System control
    writeReg(ES8311_REG10, 0x1F);  // System control: reset
    writeReg(ES8311_REG11, 0x7F);  // System control
    writeReg(ES8311_REG00, 0x80);  // Reset register
    delay(10);

    // Power up analog
    writeReg(ES8311_REG0D, 0x01);  // Power up ADC analog
    writeReg(ES8311_REG0E, 0x02);  // Enable analog PGA

    // System configuration
    writeReg(ES8311_REG12, 0x00);  // System config
    writeReg(ES8311_REG14, 0x1A);  // Microphone bias and DMIC config

    // ADC path configuration
    writeReg(ES8311_REG16, 0x24);  // ADC scale (gain)
    writeReg(ES8311_REG17, 0xBF);  // ADC enable/channel config
    writeReg(ES8311_REG15, 0x40);  // ADC mute control (unmuted)

    // ALC (Automatic Level Control) - optional
    writeReg(ES8311_REG10, 0x00);  // ALC disabled for now
    writeReg(ES8311_REG11, 0x00);  // ALC max gain
    writeReg(ES8311_REG13, 0x00);  // ALC control

    // GPIO and reference signals
    writeReg(ES8311_REG44, 0x08);  // Internal reference: normal

    // I2S format for ADC
    writeReg(ES8311_REG09, 0x00);  // ADC I2S format: standard I2S, 16-bit
    writeReg(ES8311_REG0A, 0x00);  // ADC I2S mode
}

void ES8311::configureDAC() {
    // DAC settings
    writeReg(ES8311_REG17, 0xBF);  // DAC power up
    writeReg(ES8311_REG18, 0x00);  // DAC I2S format: standard I2S
    writeReg(ES8311_REG19, 0x00);  // DAC word length: 16-bit
    writeReg(ES8311_REG1A, 0x00);  // DAC volume: 0dB
    writeReg(ES8311_REG1B, 0x00);  // DAC normal mode
    writeReg(ES8311_REG1C, 0x00);  // DAC output: differential

    // DAC volume control registers
    writeReg(ES8311_REG31, 0x00);  // DAC volume left
    writeReg(ES8311_REG32, 0x00);  // DAC volume right
}

void ES8311::setMicGain(ES8311_MicGain gain) {
    // ADC PGA gain (0-42dB in 6dB steps)
    uint8_t gain_val = (uint8_t)gain << 4;
    writeReg(ES8311_REG0E, 0x02 | gain_val);
    Serial.printf("[ES8311] Mic gain set to %ddB\n", (int)gain * 6);
}

void ES8311::setVolume(uint8_t volume) {
    if (volume > 100) volume = 100;

    // Convert 0-100 to ES8311 volume range
    // ES8311 volume is from 0 (max) to 255 (-95.5dB)
    // Inverted: 0 = max volume, 255 = min volume
    uint8_t dacVol = (uint8_t)((100 - volume) * 255 / 100);

    writeReg(ES8311_REG31, dacVol);  // Left channel
    writeReg(ES8311_REG32, dacVol);  // Right channel

    Serial.printf("[ES8311] Volume set to %d%%\n", volume);
}

void ES8311::muteMic(bool mute) {
    if (mute) {
        writeReg(ES8311_REG15, 0xC0);  // Mute ADC (bits 7:6 = 11)
        Serial.println("[ES8311] Microphone muted");
    } else {
        writeReg(ES8311_REG15, 0x40);  // Unmute ADC (bits 7:6 = 01)
        Serial.println("[ES8311] Microphone unmuted");
    }
}

void ES8311::muteDAC(bool mute) {
    if (mute) {
        writeReg(ES8311_REG1A, 0x03);  // Mute DAC
        Serial.println("[ES8311] Speaker muted");
    } else {
        writeReg(ES8311_REG1A, 0x00);  // Unmute DAC
        Serial.println("[ES8311] Speaker unmuted");
    }
}

void ES8311::powerDown() {
    Serial.println("[ES8311] Power down");
    writeReg(ES8311_REG00, 0x1F);  // Power down all modules
}

void ES8311::powerUp() {
    Serial.println("[ES8311] Power up");
    writeReg(ES8311_REG00, 0x00);  // Release power down
    delay(10);

    // Reconfigure
    configureClocks(sample_rate_);
    configureADC();
    configureDAC();
}

bool ES8311::writeReg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(value);
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        Serial.printf("[ES8311] I2C write error: %d (reg 0x%02X)\n", error, reg);
        return false;
    }
    return true;
}

uint8_t ES8311::readReg(uint8_t reg) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(ES8311_ADDR, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }

    Serial.printf("[ES8311] I2C read error (reg 0x%02X)\n", reg);
    return 0;
}
