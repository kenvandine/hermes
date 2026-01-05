#!/bin/bash
# HERMES Build Script
# This script allows you to configure your HERMES device at build time
# by setting environment variables instead of hardcoding values.

# Usage:
#   ./build.sh                                    - Build for Waveshare (default)
#   ./build.sh upload                             - Build and upload for Waveshare
#   PLATFORM=nabu_casa ./build.sh upload          - Build and upload for Nabu Casa
#   ROOM_NAME="Kitchen" ./build.sh upload         - Build with custom room name
#   PLATFORM=nabu_casa ROOM_NAME="Kitchen" ./build.sh upload - Nabu Casa with custom config

# ============================================================================
# Configuration Defaults
# Override any of these by setting environment variables before running
# For a complete example, see .env.example
# ============================================================================

# Device Platform (waveshare or nabu_casa)
export PLATFORM="${PLATFORM:-waveshare}"

# Room Configuration
export ROOM_NAME="${ROOM_NAME:-Unconfigured}"

# WiFi Configuration
export WIFI_SSID="${WIFI_SSID:-your-wifi-ssid}"
export WIFI_PASSWORD="${WIFI_PASSWORD:-your-wifi-password}"

# MQTT Configuration (Home Assistant)
export MQTT_BROKER="${MQTT_BROKER:-your-mqtt-broker-ip}"
export MQTT_PORT="${MQTT_PORT:-1883}"
export MQTT_USERNAME="${MQTT_USERNAME:-homeassistant}"
export MQTT_PASSWORD="${MQTT_PASSWORD:-your-mqtt-password}"

# AI Services Configuration (for future use when audio features are enabled)
export OLLAMA_HOST="${OLLAMA_HOST:-http://your-server-ip:11434}"
export PIPER_HOST="${PIPER_HOST:-http://your-server-ip:10200}"
export HA_HOST="${HA_HOST:-http://your-server-ip:8123}"
export HA_TOKEN="${HA_TOKEN:-your_ha_token_here}"

# ============================================================================
# Display Configuration
# ============================================================================

echo "========================================"
echo "  HERMES Build Configuration"
echo "========================================"
echo ""
echo "Device Platform:      $PLATFORM"
echo ""
echo "Room Configuration:"
echo "  Room Name:          $ROOM_NAME"
echo ""
echo "WiFi Configuration:"
echo "  SSID:               $WIFI_SSID"
echo "  Password:           ********"
echo ""
echo "MQTT Configuration:"
echo "  Broker:             $MQTT_BROKER:$MQTT_PORT"
echo "  Username:           $MQTT_USERNAME"
echo "  Password:           ********"
echo ""
echo "AI Services:"
echo "  Ollama:             $OLLAMA_HOST"
echo "  Piper:              $PIPER_HOST"
echo "  Home Assistant:     $HA_HOST"
if [ "$HA_TOKEN" != "your_ha_token_here" ]; then
    echo "  HA Token:           ********"
else
    echo "  HA Token:           not configured"
fi
echo ""
echo "========================================"
echo ""

# ============================================================================
# Build Process
# ============================================================================

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Use the local .venv if it exists, otherwise use system platformio
if [ -d "$SCRIPT_DIR/.venv" ]; then
    PIO="$SCRIPT_DIR/.venv/bin/pio"
else
    PIO="pio"
fi

# Generate build configuration from environment variables
echo "Generating build configuration..."
python3 "$SCRIPT_DIR/scripts/generate_config.py"
if [ $? -ne 0 ]; then
    echo "✗ Failed to generate build configuration!"
    exit 1
fi
echo ""

# Determine the target (upload or just build)
TARGET="${1:-run}"

if [ "$TARGET" = "upload" ]; then
    echo "Building and uploading to device..."
    "$PIO" run -e "$PLATFORM" --target upload
else
    echo "Building firmware..."
    "$PIO" run -e "$PLATFORM"
fi

# Check if build was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "  ✓ Build Successful!"
    echo "========================================"
    if [ "$TARGET" = "upload" ]; then
        echo ""
        echo "Firmware has been uploaded to the device."
        echo "The device should now be running with your configuration."
        echo ""
        echo "To monitor serial output:"
        echo "  pio device monitor"
    else
        echo ""
        echo "To upload to device, run:"
        echo "  ./build.sh upload"
    fi
    echo ""
else
    echo ""
    echo "========================================"
    echo "  ✗ Build Failed!"
    echo "========================================"
    echo ""
    echo "Please check the error messages above."
    exit 1
fi
