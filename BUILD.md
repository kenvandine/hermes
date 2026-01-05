# Building HERMES

This document explains how to build and upload the HERMES firmware to your ESP32-S3 device.

## Prerequisites

- Python 3.x
- PlatformIO (installed in `.venv` or system-wide)
- ESP32-S3 device connected via USB

## Supported Devices

HERMES supports multiple hardware platforms:

- **Waveshare ESP32-S3 1.8" AMOLED Touch** (default)
  - Full touchscreen UI with LVGL
  - 368×448 AMOLED display
  - Capacitive touch control

- **Nabu Casa Voice PE**
  - LED ring visual feedback (12 WS2812B LEDs)
  - Physical controls (rotary encoder, button, mute switch)
  - No display UI

## Quick Start

The easiest way to build and upload is to use the `build.sh` script:

```bash
# Build and upload for Waveshare (default)
./build.sh upload

# Build and upload for Nabu Casa Voice PE
PLATFORM=nabu_casa ./build.sh upload

# Just build (don't upload)
./build.sh
```

## Configuration

The build system uses environment variables to configure your device. This means you don't need to hardcode credentials in the source code.

### Default Values

If you don't set any environment variables, the following defaults are used:

| Variable | Default Value |
|----------|--------------|
| `ROOM_NAME` | Unconfigured |
| `WIFI_SSID` | your-wifi-ssid |
| `WIFI_PASSWORD` | your-wifi-password |
| `MQTT_BROKER` | your-mqtt-broker-ip |
| `MQTT_PORT` | 1883 |
| `MQTT_USERNAME` | homeassistant |
| `MQTT_PASSWORD` | your-mqtt-password |
| `OLLAMA_HOST` | http://your-server-ip:11434 |
| `PIPER_HOST` | http://your-server-ip:10200 |
| `HA_HOST` | http://your-server-ip:8123 |
| `HA_TOKEN` | your_ha_token_here |

### Custom Configuration

You can override any of these values by setting environment variables before running the build script:

```bash
# Single variable
ROOM_NAME="Kitchen" ./build.sh upload

# Multiple variables
ROOM_NAME="Bedroom" \
WIFI_SSID="my-network" \
WIFI_PASSWORD="my-password" \
MQTT_BROKER="192.168.1.100" \
./build.sh upload
```

### Using a Configuration File

For convenience, you can create a configuration file and source it before building:

```bash
# Create a config file (do NOT commit this to git!)
cat > .env <<EOF
export ROOM_NAME="Kitchen"
export WIFI_SSID="my-network"
export WIFI_PASSWORD="my-password"
export MQTT_BROKER="192.168.1.100"
export MQTT_PORT="1883"
export MQTT_USERNAME="homeassistant"
export MQTT_PASSWORD="my-secure-password"
export OLLAMA_HOST="http://192.168.1.100:11434"
export PIPER_HOST="http://192.168.1.100:10200"
export HA_HOST="http://192.168.1.100:8123"
export HA_TOKEN="your-long-lived-access-token-here"
EOF

# Source the config and build
source .env && ./build.sh upload
```

**Important:** Add `.env` to your `.gitignore` to avoid committing credentials!

### Home Assistant Voice Control Integration

To enable voice control of Home Assistant devices, you need to configure a long-lived access token:

1. **Create a long-lived access token** in Home Assistant:
   - Go to your Home Assistant profile: **Settings** → **Profile** → **Long-Lived Access Tokens**
   - Click **"Create Token"**
   - Give it a name like "HERMES Voice Control"
   - Copy the token (you won't be able to see it again!)

2. **Set the token in your configuration**:
   ```bash
   export HA_HOST="http://192.168.1.100:8123"
   export HA_TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
   ```

3. **Build with the token**:
   ```bash
   HA_HOST="http://192.168.1.100:8123" \
   HA_TOKEN="your-actual-token" \
   ./build.sh upload
   ```

Now you can use voice commands like:
- "Hey Hermes, turn on kitchen light"
- "Hey Hermes, set living room temperature to 72"
- "Hey Hermes, dim bedroom lights to 50%"

The system will automatically route device control commands to Home Assistant, while keeping general questions routed to the Ollama AI assistant.

## Build Process

When you run `build.sh`, the following happens:

1. **Configuration Display**: Shows the current configuration (from environment variables)
2. **Config Generation**: Runs `scripts/generate_config.py` to create `include/build_config.h`
3. **Compilation**: PlatformIO compiles the firmware with your configuration
4. **Upload** (if requested): Uploads the firmware to your ESP32-S3 device

## Building for Different Devices

### Using build.sh (Recommended)

```bash
# Waveshare (default)
./build.sh upload

# Nabu Casa Voice PE
PLATFORM=nabu_casa ./build.sh upload
```

### Using PlatformIO Directly

```bash
# Waveshare ESP32-S3 AMOLED Touch
.venv/bin/pio run -e waveshare
.venv/bin/pio run -e waveshare --target upload

# Nabu Casa Voice PE
.venv/bin/pio run -e nabu_casa
.venv/bin/pio run -e nabu_casa --target upload
```

## Manual Build Steps

If you prefer to run the steps manually:

```bash
# 1. Set environment variables
export ROOM_NAME="Living Room"
export WIFI_SSID="my-network"
export PLATFORM="waveshare"  # or "nabu_casa"
# ... etc ...

# 2. Generate configuration
python3 scripts/generate_config.py

# 3. Build for specific device
.venv/bin/pio run -e waveshare  # or -e nabu_casa

# 4. Upload
.venv/bin/pio run -e waveshare --target upload  # or -e nabu_casa
```

## Monitoring Serial Output

After uploading, you can monitor the device's serial output:

```bash
.venv/bin/pio device monitor
```

Or use the shorthand:

```bash
pio device monitor
```

Press `Ctrl+C` to exit the monitor.

## Troubleshooting

### Build fails with "pio: command not found"

The build script looks for PlatformIO in `.venv/bin/pio`. If you installed PlatformIO differently, you may need to:

1. Install PlatformIO in a virtual environment:
   ```bash
   python3 -m venv .venv
   .venv/bin/pip install platformio
   ```

2. Or modify `build.sh` to use your PlatformIO installation

### Upload fails with "No such port"

Make sure your ESP32-S3 device is connected via USB and you have permissions to access it:

```bash
# Add yourself to the dialout group (Linux)
sudo usermod -a -G dialout $USER
# Log out and log back in for this to take effect
```

### Device doesn't connect to WiFi

1. Check that your WiFi SSID and password are correct
2. Make sure you're using a 2.4GHz network (ESP32 doesn't support 5GHz)
3. Monitor serial output to see connection attempts:
   ```bash
   pio device monitor
   ```

### MQTT connection fails

1. Verify your MQTT broker IP address and port
2. Check MQTT username and password
3. Ensure your MQTT broker allows connections from the ESP32's IP
4. Check serial output for MQTT error messages

## Device-Specific Features

### Waveshare ESP32-S3 AMOLED Touch

- **Display**: 1.8" AMOLED (368×448) with full LVGL UI
- **Input**: Capacitive touch screen
- **Audio**: External I2S microphone and speaker required
- **Visual Feedback**: Full-color UI with device list and call status
- **Pin Configuration**: See `docs/hardware_setup_waveshare_amoled.md`

### Nabu Casa Voice PE

- **Display**: 12-LED WS2812B ring (visual state indicators)
- **Input**: Rotary encoder (volume), action button (mute), hardware mute switch
- **Audio**: Built-in AIC3204 codec with XMOS USB audio processor
- **Visual Feedback**:
  - LED colors: Blue (idle), Green (listening/active), Red (error/muted)
  - Volume display: LED count shows volume level (0-12 LEDs = 0-100%)
  - Color-coded volume: Cyan (normal), Orange (high), Red (muted)
- **Pin Configuration**: Pre-configured in `include/devices/nabu_casa/device_pins.h`

## Advanced

### Building for Different Environments

You can create different configuration files for different rooms:

```bash
# kitchen.env
export ROOM_NAME="Kitchen"
# ... kitchen-specific config ...

# bedroom.env
export ROOM_NAME="Bedroom"
# ... bedroom-specific config ...

# Build for kitchen
source kitchen.env && ./build.sh upload

# Build for bedroom
source bedroom.env && ./build.sh upload
```

### Continuous Integration

The build system is CI-friendly since configuration is done via environment variables:

```yaml
# Example GitHub Actions workflow
- name: Build HERMES
  env:
    ROOM_NAME: ${{ secrets.ROOM_NAME }}
    WIFI_SSID: ${{ secrets.WIFI_SSID }}
    WIFI_PASSWORD: ${{ secrets.WIFI_PASSWORD }}
    MQTT_BROKER: ${{ secrets.MQTT_BROKER }}
    # ... etc ...
  run: ./build.sh
```

## Files Generated During Build

- `include/build_config.h` - Auto-generated configuration header (do NOT edit manually)
- `.pio/` - PlatformIO build artifacts (automatically created)

Both of these should be in `.gitignore` and not committed to version control.
