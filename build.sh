#!/bin/bash
# HERMES Build Script
# This script allows you to configure your HERMES device at build time
# by setting environment variables instead of hardcoding values.

# Usage:
#   ./build.sh                    - Build with default values
#   ./build.sh upload             - Build and upload to device
#   ROOM_NAME="Kitchen" ./build.sh upload - Build with custom room name and upload

# ============================================================================
# Configuration Defaults
# Override any of these by setting environment variables before running
# For a complete example, see .env.example
# ============================================================================

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

# ============================================================================
# Display Configuration
# ============================================================================

echo "========================================"
echo "  HERMES Build Configuration"
echo "========================================"
echo ""
echo "Room Configuration:"
echo "  Room Name:        $ROOM_NAME"
echo ""
echo "WiFi Configuration:"
echo "  SSID:             $WIFI_SSID"
echo "  Password:         ********"
echo ""
echo "MQTT Configuration:"
echo "  Broker:           $MQTT_BROKER:$MQTT_PORT"
echo "  Username:         $MQTT_USERNAME"
echo "  Password:         ********"
echo ""
echo "AI Services (for future use):"
echo "  Ollama:           $OLLAMA_HOST"
echo "  Piper:            $PIPER_HOST"
echo "  Home Assistant:   $HA_HOST"
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
    "$PIO" run --target upload
else
    echo "Building firmware..."
    "$PIO" run
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
