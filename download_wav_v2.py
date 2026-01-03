#!/usr/bin/env python3
"""
Download WAV file from ESP32 - looks for end marker instead of timeout
"""
import serial
import time
import sys

SERIAL_PORT = '/dev/ttyACM0'
BAUD_RATE = 115200

def main():
    print(f"Opening {SERIAL_PORT} at {BAUD_RATE} baud...")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)

        # First, record audio
        print("\nSending 'r' command to record audio...")
        ser.write(b'r')
        ser.flush()
        time.sleep(3)  # Wait for recording to complete

        # Clear any pending output
        while ser.in_waiting:
            ser.readline()

        # Now download
        print("\nSending 'd' command to download WAV...")
        ser.write(b'd')
        ser.flush()
        time.sleep(0.5)

        # Collect data until we see the end marker
        print("Collecting data (looking for end marker)...")
        all_lines = []
        start_found = False
        end_found = False

        max_wait = 30  # 30 seconds max
        start_time = time.time()

        while not end_found and (time.time() - start_time) < max_wait:
            if ser.in_waiting:
                line = ser.readline()
                all_lines.append(line)

                line_text = line.decode('utf-8', errors='ignore')

                if '--- BEGIN WAV FILE ---' in line_text:
                    start_found = True
                    print(f"Found start marker at line {len(all_lines)}")

                if '--- END WAV FILE ---' in line_text:
                    end_found = True
                    print(f"Found end marker at line {len(all_lines)}")
                    break
            else:
                time.sleep(0.01)

        ser.close()

        if not start_found:
            print("\n✗ Never found start marker")
            print(f"Collected {len(all_lines)} lines")
            print("First 10 lines:")
            for i, line in enumerate(all_lines[:10]):
                print(f"  {i}: {line[:80]}")
            return

        if not end_found:
            print("\n✗ Found start but not end marker (timeout)")
            return

        # Reassemble and extract WAV data
        print("\nExtracting WAV data...")
        all_data = b''.join(all_lines)

        start_marker = b'--- BEGIN WAV FILE ---'
        end_marker = b'--- END WAV FILE ---'

        start_idx = all_data.find(start_marker)
        end_idx = all_data.find(end_marker)

        # WAV data is between markers (skip the newline after start marker)
        wav_start = start_idx + len(start_marker)
        while wav_start < len(all_data) and all_data[wav_start:wav_start+1] in (b'\n', b'\r'):
            wav_start += 1

        wav_data = all_data[wav_start:end_idx]

        # Save to file
        output_file = '/tmp/recording.wav'
        with open(output_file, 'wb') as f:
            f.write(wav_data)

        print(f"\n✓ Saved {len(wav_data)} bytes to {output_file}")
        print(f"\nTo play: aplay {output_file}")
        print(f"To analyze: ffprobe {output_file}")

        # Quick validation
        if wav_data[:4] == b'RIFF':
            print("✓ WAV header looks correct (starts with RIFF)")
        else:
            print(f"✗ WAV header looks wrong (starts with {wav_data[:4]})")

    except Exception as e:
        print(f"\n✗ Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
