#!/usr/bin/env python3
"""
Bump versions for both OBD-II Emulator and Proxy firmware
"""

import subprocess
import sys
from pathlib import Path

def main():
    script_dir = Path(__file__).parent
    bump_script = script_dir / "bump_emulator_version.py"
    
    print("Bumping versions for both emulator and proxy firmware...")
    
    # Bump emulator version
    print("\n=== Bumping Emulator Version ===")
    try:
        subprocess.run([sys.executable, str(bump_script), "emulator"], check=True)
        print("✅ Emulator version bumped successfully")
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to bump emulator version: {e}")
        return 1
    
    # Bump proxy version
    print("\n=== Bumping Proxy Version ===")
    try:
        subprocess.run([sys.executable, str(bump_script), "proxy"], check=True)
        print("✅ Proxy version bumped successfully")
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to bump proxy version: {e}")
        return 1
    
    print("\n🎉 All versions bumped successfully!")
    return 0

if __name__ == "__main__":
    exit(main()) 