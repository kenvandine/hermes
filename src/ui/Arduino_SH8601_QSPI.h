/*
 * Arduino_SH8601_QSPI - QSPI Bus with SH8601 Opcodes for Arduino_GFX
 * Implements the SH8601-specific QSPI protocol with command/data opcodes
 */

#ifndef _ARDUINO_SH8601_QSPI_H_
#define _ARDUINO_SH8601_QSPI_H_

#include <Arduino_DataBus.h>
#include <driver/spi_master.h>

#define LCD_OPCODE_WRITE_CMD   0x02  // Write command opcode
#define LCD_OPCODE_WRITE_COLOR 0x32  // Write color data opcode

class Arduino_SH8601_QSPI : public Arduino_DataBus
{
public:
    Arduino_SH8601_QSPI(int8_t cs, int8_t sck, int8_t d0, int8_t d1, int8_t d2, int8_t d3);

    bool begin(int32_t speed = GFX_NOT_DEFINED, int8_t dataMode = GFX_NOT_DEFINED) override;
    void beginWrite() override;
    void endWrite() override;
    void writeCommand(uint8_t c) override;
    void writeCommand16(uint16_t c) override;
    void write(uint8_t d) override;
    void write16(uint16_t d) override;
    void writeRepeat(uint16_t p, uint32_t len) override;
    void writeBytes(uint8_t *data, uint32_t len) override;
    void writePixels(uint16_t *data, uint32_t len) override;

protected:
    int8_t _cs, _sck, _d0, _d1, _d2, _d3;
    spi_device_handle_t _handle;
    bool _isInit = false;

private:
    void writeWithOpcode(uint32_t opcode, const uint8_t *data, size_t len);
};

#endif // _ARDUINO_SH8601_QSPI_H_
