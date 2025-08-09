#!/usr/bin/env python3
"""
Version bump script for OBD-II Emulator and Proxy firmware
Updates version numbers in source files and OTA JSON files
"""

import re
import json
import subprocess
import argparse
from pathlib import Path

def bump_version(firmware_type):
    """Bump version for specified firmware type (emulator or proxy)"""
    
    # File paths
    ino_path = Path(f"src/OBD2{firmware_type.capitalize()}.cpp")
    json_path = Path(f"ota/{firmware_type}.json")
    firmware_src = Path(f".pio/build/esp32-{firmware_type}/firmware.bin")
    firmware_dst = Path(f"ota/firmware/{firmware_type}.bin")
    
    print(f"Bumping version for {firmware_type} firmware...")
    
    # Step 1: Update ota_version in source file
    if not ino_path.exists():
        raise FileNotFoundError(f"Source file {ino_path} not found")
    
    ino_content = ino_path.read_text()
    
    match = re.search(r'(ota_version\s*=\s*")(\d+)\.(\d+)\.(\d+)(")', ino_content)
    if not match:
        raise ValueError(f"ota_version string not found in {ino_path}")
    
    major, minor, patch = map(int, match.group(2, 3, 4))
    patch += 1
    new_version = f"{major}.{minor}.{patch}"
    
    updated_ino = re.sub(
        r'(ota_version\s*=\s*")(\d+)\.(\d+)\.(\d+)(")',
        lambda m: f'{m.group(1)}{new_version}{m.group(5)}',
        ino_content
    )
    ino_path.write_text(updated_ino)
    print(f"Updated {ino_path} to version {new_version}")
    
    # Step 2: Update version in OTA JSON file
    if not json_path.exists():
        raise FileNotFoundError(f"OTA JSON file {json_path} not found")
    
    with open(json_path, "r") as f:
        ota_data = json.load(f)
    
    ota_data["Configurations"][0]["Version"] = new_version
    
    with open(json_path, "w") as f:
        json.dump(ota_data, f, indent=2)
    
    print(f"Updated {json_path} to version {new_version}")
    
    # Step 3: Build firmware
    print(f"Building {firmware_type} firmware...")
    subprocess.run(["platformio", "run", "-e", f"esp32-{firmware_type}"], check=True)
    
    # Step 4: Copy firmware binary
    if not firmware_src.exists():
        raise FileNotFoundError(f"{firmware_src} not found after build")
    
    # Ensure firmware directory exists
    firmware_dst.parent.mkdir(parents=True, exist_ok=True)
    
    firmware_dst.write_bytes(firmware_src.read_bytes())
    print(f"Copied firmware to {firmware_dst}")
    
    print(f"✅ {firmware_type} version bump completed successfully!")

def main():
    parser = argparse.ArgumentParser(description="Bump version for OBD-II firmware")
    parser.add_argument("type", choices=["emulator", "proxy"], 
                       help="Firmware type to bump version for")
    
    args = parser.parse_args()
    
    try:
        bump_version(args.type)
    except Exception as e:
        print(f"❌ Error: {e}")
        exit(1)

if __name__ == "__main__":
    main()

