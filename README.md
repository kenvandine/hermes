# ESP32 Multi-Room Intercom System

A voice-activated, multi-room intercom system built on ESP32-S3 with full Home Assistant integration.

## Features

- **Voice Control**: Wake word detection ("Hey Intercom") with voice commands
- **Multi-Room Communication**: Drop in on any room in your house
- **Touch Screen UI**: 1.8" AMOLED display (368×448) with capacitive touch
- **Home Assistant Integration**: Full MQTT Discovery, automation triggers, and dashboard controls
- **Low Latency Audio**: Opus codec over UDP for real-time communication
- **Hands-Free Operation**: Complete call lifecycle via voice or touch
- **Compact Design**: All-in-one board with integrated display and touch

## Hardware Requirements

### Required Components
- **Waveshare ESP32-S3 1.8" AMOLED Touch Display** (368×448, RM67162 controller)
  - Includes: ESP32-S3-WROOM-1, AMOLED display, CST816S touch
  - Built-in: 16MB Flash, 8MB PSRAM, USB-C
- **I2S MEMS Microphone**: INMP441 or equivalent
- **I2S Audio Amplifier**: MAX98357A or equivalent
- **Speaker**: 4-8Ω, 3W (compact speaker recommended)
- **Jumper wires**: For connecting audio components

### Optional Components
- 3.7V LiPo battery (for portable use)
- Custom enclosure (45mm × 70mm × 20mm minimum)
- Acoustic foam (for mic/speaker isolation)

## Software Requirements

- PlatformIO (VS Code extension or CLI)
- Home Assistant with MQTT broker (Mosquitto addon)
- Edge Impulse account (for wake word training)

## Quick Start

### 1. Clone and Build

```bash
git clone https://github.com/kenvandine/esp32-intercom.git
cd esp32-intercom
pio run
```

### 2. Assemble Hardware

Follow the comprehensive wiring guide in `docs/hardware_setup_waveshare_amoled.md`:
- Connect INMP441 microphone to GPIOs 1, 2, 42
- Connect MAX98357A amplifier to GPIOs 3, 4, 5
- Connect speaker to MAX98357A output
- Pins are pre-configured in `include/config.h` for Waveshare ESP32-S3 1.8" AMOLED

### 3. First Boot Setup

1. Flash the firmware: `pio run -t upload`
2. Device creates WiFi AP "Intercom-Setup"
3. Connect and enter your WiFi credentials
4. Enter room name on touch screen
5. Device auto-discovers Home Assistant MQTT broker

### 4. Train Wake Word (Optional - Phase 5)

1. Create Edge Impulse project
2. Record wake word samples ("Hey Intercom")
3. Train model and export for Arduino
4. Place model files in `data/edge_impulse_model/`

## Home Assistant Integration

Devices automatically appear in Home Assistant via MQTT Discovery. No manual configuration needed!

### Example Automation

```yaml
automation:
  - alias: "Doorbell to Kitchen Intercom"
    trigger:
      - platform: state
        entity_id: binary_sensor.front_door_doorbell
        to: 'on'
    action:
      - service: mqtt.publish
        data:
          topic: intercom/devices/esp32_kitchen/call/initiate
          payload: '{"target_room": "Kitchen"}'
```

### Example Dashboard Card

```yaml
type: entities
title: Intercom System
entities:
  - entity: binary_sensor.kitchen_intercom_call
  - entity: sensor.kitchen_intercom_caller
  - entity: binary_sensor.living_room_intercom_call
```

## Architecture

- **Signaling**: MQTT for device discovery and call setup
- **Audio**: Opus codec over UDP for low-latency streaming
- **UI**: LVGL graphics library on SPI display
- **Storage**: NVS for persistent configuration

## Development Phases

- **Phase 1**: Foundation (WiFi, I2S, Touch Screen) ✅
- **Phase 2**: MQTT & HA Discovery
- **Phase 3**: Audio Streaming
- **Phase 4**: Call Management
- **Phase 5**: Wake Word Detection
- **Phase 6**: Voice Commands
- **Phase 7**: Polish & Stability
- **Phase 8**: Future Enhancements (Ollama AI, etc.)

## Pin Configuration

Default pin configuration in `include/config.h`:

| Function | GPIO |
|----------|------|
| I2S MIC SCK | 26 |
| I2S MIC WS | 25 |
| I2S MIC SD | 33 |
| I2S SPK SCK | 14 |
| I2S SPK WS | 12 |
| I2S SPK SD | 13 |
| Display SPI | See config.h |
| Touch SPI | See config.h |

## Voice Commands

- "Hey Intercom" - Wake word to activate listening
- "Drop in on [Room Name]" - Initiate call to room
- "Hang up" - End active call
- "Cancel" - Cancel command listening

## Contributing

Contributions welcome! Please open issues or pull requests.

## License

MIT License - See LICENSE file

## Acknowledgments

- [arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools) by Phil Schatzmann
- [LVGL](https://lvgl.io/) for graphics library
- [Edge Impulse](https://edgeimpulse.com/) for wake word detection
- Home Assistant community for MQTT Discovery protocol

## Support

For issues, questions, or feature requests, please open a GitHub issue.
