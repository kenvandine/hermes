/**
 * ES8311 Audio Codec Driver
 *
 * I2C-controlled low-power audio codec with ADC and DAC
 * Used in Waveshare ESP32-S3-Touch-AMOLED-1.8" for built-in mic/speaker
 *
 * I2C Address: 0x18 (default)
 * Sample Rates: 8kHz - 96kHz
 * Input: Built-in microphone → ADC → I2S
 * Output: I2S → DAC → Built-in speaker
 */

#ifndef ES8311_H
#define ES8311_H

#include <Arduino.h>
#include <Wire.h>

// ES8311 I2C Address
#define ES8311_ADDR         0x18

// ES8311 Register Addresses
#define ES8311_REG00        0x00  // Reset control
#define ES8311_REG01        0x01  // Clock management
#define ES8311_REG02        0x02  // Clock management
#define ES8311_REG03        0x03  // Clock management
#define ES8311_REG04        0x04  // Clock management
#define ES8311_REG05        0x05  // System control
#define ES8311_REG06        0x06  // System control
#define ES8311_REG07        0x07  // System control
#define ES8311_REG08        0x08  // System control
#define ES8311_REG09        0x09  // ADC control
#define ES8311_REG0A        0x0A  // ADC control
#define ES8311_REG0B        0x0B  // ADC control
#define ES8311_REG0C        0x0C  // ADC control
#define ES8311_REG0D        0x0D  // ADC control
#define ES8311_REG0E        0x0E  // ADC control
#define ES8311_REG0F        0x0F  // ADC control
#define ES8311_REG10        0x10  // ALC control
#define ES8311_REG11        0x11  // ALC control
#define ES8311_REG12        0x12  // ALC control
#define ES8311_REG13        0x13  // ALC control
#define ES8311_REG14        0x14  // ALC control
#define ES8311_REG15        0x15  // ALC control
#define ES8311_REG16        0x16  // ADC mute
#define ES8311_REG17        0x17  // DAC control
#define ES8311_REG18        0x18  // DAC control
#define ES8311_REG19        0x19  // DAC control
#define ES8311_REG1A        0x1A  // DAC control
#define ES8311_REG1B        0x1B  // DAC control
#define ES8311_REG1C        0x1C  // DAC control
#define ES8311_REG31        0x31  // DAC volume
#define ES8311_REG32        0x32  // DAC volume
#define ES8311_REG37        0x37  // DAC control
#define ES8311_REG44        0x44  // GPIO control
#define ES8311_REG45        0x45  // GPIO control

// Audio input/output modes
enum ES8311_MicGain {
    GAIN_0DB = 0,
    GAIN_6DB,
    GAIN_12DB,
    GAIN_18DB,
    GAIN_24DB,
    GAIN_30DB,
    GAIN_36DB,
    GAIN_42DB
};

class ES8311 {
public:
    ES8311();

    /**
     * Initialize ES8311 codec
     * @param i2c_sda I2C data pin
     * @param i2c_scl I2C clock pin
     * @param sample_rate Audio sample rate (8000-96000 Hz)
     * @return true if successful
     */
    bool begin(int i2c_sda, int i2c_scl, uint32_t sample_rate = 16000);

    /**
     * Set microphone input gain
     * @param gain Gain level (GAIN_0DB to GAIN_42DB)
     */
    void setMicGain(ES8311_MicGain gain);

    /**
     * Set speaker output volume
     * @param volume Volume level (0-100)
     */
    void setVolume(uint8_t volume);

    /**
     * Mute/unmute microphone
     * @param mute true to mute, false to unmute
     */
    void muteMic(bool mute);

    /**
     * Mute/unmute speaker
     * @param mute true to mute, false to unmute
     */
    void muteDAC(bool mute);

    /**
     * Power down codec (low power mode)
     */
    void powerDown();

    /**
     * Power up codec
     */
    void powerUp();

private:
    uint32_t sample_rate_;

    /**
     * Write single byte to ES8311 register
     */
    bool writeReg(uint8_t reg, uint8_t value);

    /**
     * Read single byte from ES8311 register
     */
    uint8_t readReg(uint8_t reg);

    /**
     * Configure clocking for sample rate
     */
    void configureClocks(uint32_t sample_rate);

    /**
     * Configure ADC (microphone input)
     */
    void configureADC();

    /**
     * Configure DAC (speaker output)
     */
    void configureDAC();
};

#endif // ES8311_H
