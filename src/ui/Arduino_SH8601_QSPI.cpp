/*
 * Arduino_SH8601_QSPI - QSPI Bus with SH8601 Opcodes for Arduino_GFX
 */

#include "Arduino_SH8601_QSPI.h"

Arduino_SH8601_QSPI::Arduino_SH8601_QSPI(int8_t cs, int8_t sck, int8_t d0, int8_t d1, int8_t d2, int8_t d3)
    : _cs(cs), _sck(sck), _d0(d0), _d1(d1), _d2(d2), _d3(d3)
{
}

bool Arduino_SH8601_QSPI::begin(int32_t speed, int8_t dataMode)
{
    if (_isInit) {
        return true;
    }

    // Initialize QSPI using ESP-IDF SPI driver
    spi_bus_config_t buscfg = {
        .mosi_io_num = _d0,
        .miso_io_num = -1,
        .sclk_io_num = _sck,
        .quadwp_io_num = _d2,
        .quadhd_io_num = _d3,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 65536,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_QUAD,
        .intr_flags = 0
    };

    spi_device_interface_config_t devcfg = {
        .command_bits = 8,  // 8-bit command for opcode
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = 10000000,  // Reduce to 10MHz for testing
        .input_delay_ns = 0,
        .spics_io_num = _cs,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 1,
        .pre_cb = nullptr,
        .post_cb = nullptr
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        Serial.printf("[SH8601_QSPI] Bus init failed: %d\n", ret);
        return false;
    }

    // Add device to SPI bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &_handle);
    if (ret != ESP_OK) {
        Serial.printf("[SH8601_QSPI] Device add failed: %d\n", ret);
        return false;
    }

    _isInit = true;
    Serial.println("[SH8601_QSPI] ✓ Initialized successfully");
    return true;
}

void Arduino_SH8601_QSPI::beginWrite()
{
    // CS handled by SPI driver
}

void Arduino_SH8601_QSPI::endWrite()
{
    // CS handled by SPI driver
}

void Arduino_SH8601_QSPI::writeCommand(uint8_t c)
{
    // SH8601 QSPI: Send opcode in command phase, data in QSPI data phase
    spi_transaction_t trans = {};
    trans.flags = SPI_TRANS_MODE_QIO;  // Data phase uses QSPI
    trans.cmd = LCD_OPCODE_WRITE_CMD;   // Opcode 0x02 in command phase
    trans.length = 8;                   // 1 byte of data
    trans.tx_data[0] = c;               // Command byte
    trans.flags |= SPI_TRANS_USE_TXDATA; // Use tx_data instead of tx_buffer

    spi_device_polling_transmit(_handle, &trans);
}

void Arduino_SH8601_QSPI::writeCommand16(uint16_t c)
{
    writeCommand((uint8_t)(c >> 8));
    writeCommand((uint8_t)(c & 0xFF));
}

void Arduino_SH8601_QSPI::write(uint8_t d)
{
    // Send single byte with opcode in command phase
    spi_transaction_t trans = {};
    trans.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_USE_TXDATA;
    trans.cmd = LCD_OPCODE_WRITE_COLOR;  // Opcode 0x32 in command phase
    trans.length = 8;                     // 1 byte of data
    trans.tx_data[0] = d;

    spi_device_polling_transmit(_handle, &trans);
}

void Arduino_SH8601_QSPI::write16(uint16_t d)
{
    // Send 16-bit color data: opcode in command phase, color in QSPI data phase
    spi_transaction_t trans = {};
    trans.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_USE_TXDATA;
    trans.cmd = LCD_OPCODE_WRITE_COLOR;  // Opcode 0x32 in command phase
    trans.length = 16;                    // 2 bytes of data
    trans.tx_data[0] = (uint8_t)(d >> 8); // High byte
    trans.tx_data[1] = (uint8_t)(d & 0xFF); // Low byte

    spi_device_polling_transmit(_handle, &trans);
}

void Arduino_SH8601_QSPI::writeRepeat(uint16_t p, uint32_t len)
{
    // Optimized repeat write for filling areas
    static uint8_t buffer[512];  // 256 pixels * 2 bytes

    uint8_t ph = (uint8_t)(p >> 8);
    uint8_t pl = (uint8_t)(p & 0xFF);

    // Fill buffer with repeated color
    for (int i = 0; i < 256; i++) {
        buffer[i*2] = ph;
        buffer[i*2 + 1] = pl;
    }

    while (len > 0) {
        uint32_t chunk = (len > 256) ? 256 : len;

        spi_transaction_t trans = {};
        trans.flags = SPI_TRANS_MODE_QIO;
        trans.cmd = LCD_OPCODE_WRITE_COLOR;  // Opcode 0x32 in command phase
        trans.length = chunk * 16;            // chunk pixels * 16 bits per pixel
        trans.tx_buffer = buffer;

        spi_device_polling_transmit(_handle, &trans);
        len -= chunk;
    }
}

void Arduino_SH8601_QSPI::writeBytes(uint8_t *data, uint32_t len)
{
    if (len == 0) return;

    // Send data in chunks with opcode in command phase
    uint32_t remaining = len;
    uint32_t offset = 0;

    while (remaining > 0) {
        uint32_t chunk = (remaining > 1024) ? 1024 : remaining;

        spi_transaction_t trans = {};
        trans.flags = SPI_TRANS_MODE_QIO;
        trans.cmd = LCD_OPCODE_WRITE_COLOR;  // Opcode 0x32 in command phase
        trans.length = chunk * 8;             // Data length in bits
        trans.tx_buffer = data + offset;

        spi_device_polling_transmit(_handle, &trans);

        offset += chunk;
        remaining -= chunk;
    }
}

void Arduino_SH8601_QSPI::writePixels(uint16_t *data, uint32_t len)
{
    // Convert 16-bit pixels to bytes and send
    writeBytes((uint8_t *)data, len * 2);
}
