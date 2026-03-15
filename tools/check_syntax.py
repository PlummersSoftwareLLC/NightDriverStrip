#!/usr/bin/env python3

# +--------------------------------------------------------------------------
#
# File:        check_syntax.py
#
# NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
#
# Description:
#
#    Authoritative syntax checker for NightDriver. Uses PlatformIO's 
#    compilation database to verify project files without full linking.
#
# ---------------------------------------------------------------------------

import argparse
import json
import os
import shlex
import subprocess
import sys

def run_command(cmd, cwd=None):
    """Run a shell command and return its output."""
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd, shell=isinstance(cmd, str))
    return result

def main():
    parser = argparse.ArgumentParser(description="Check project syntax using PlatformIO metadata.")
    parser.add_argument("-e", "--environment", help="PlatformIO environment to check (e.g., mesmerizer, demo_v3)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
    args = parser.parse_args()

    # 1. Determine environment
    env = args.environment
    if not env:
        # Try to find a default or pick the first one
        print("No environment specified. Pass -e <env>.")
        sys.exit(1)

    print(f"--- Preparing compilation database for environment: {env} ---")
    
    # 2. Regenerate compile_commands.json for the specified environment
    # This ensures we have the authoritative flags and compiler path.
    res = run_command(["pio", "run", "-t", "compiledb", "-e", env])
    if res.returncode != 0:
        print(f"Error: Failed to generate compilation database for '{env}'")
        print(res.stderr)
        sys.exit(1)

    # 3. Load compile_commands.json
    db_path = "compile_commands.json"
    if not os.path.exists(db_path):
        print(f"Error: {db_path} not found after generation.")
        sys.exit(1)

    with open(db_path, "r") as f:
        commands = json.load(f)

    # 4. Filter and Run
    violations = 0
    total_files = 0

    print(f"--- Auditing project files for {env} ---")

    for entry in commands:
        file_path = entry["file"]
        
        # We only care about our own files in src/ or include/
        rel_path = os.path.relpath(file_path, os.getcwd())
        if not (rel_path.startswith("src/") or rel_path.startswith("include/")):
            continue
        
        # Skip library dependencies
        if "libdeps" in file_path:
            continue

        total_files += 1
        if args.verbose:
            print(f"Checking {rel_path}...")

        cmd_str = entry["command"]
        
        # Correctly tokenize the command
        cmd_args = shlex.split(cmd_str)
        
        # Modify for syntax-only audit
        if "-c" in cmd_args:
            idx = cmd_args.index("-c")
            cmd_args[idx] = "-fsyntax-only"
        else:
            cmd_args.append("-fsyntax-only")

        # - Remove output flags
        if "-o" in cmd_args:
            idx = cmd_args.index("-o")
            cmd_args.pop(idx) # -o
            cmd_args.pop(idx) # output file
            
        # - Add extra verification flags
        cmd_args.insert(1, "-Wformat=2")
        
        # Run the audit
        res = subprocess.run(cmd_args, capture_output=True, text=True, cwd=entry["directory"])
        
        if res.returncode != 0:
            print(f"\n❌ Syntax error in {rel_path}:")
            # Filter and print output
            for line in res.stderr.splitlines():
                lower_line = line.lower()
                # Skip known external noise
                if "adafruit_gfx.h" in lower_line: continue
                if "ignoring attribute 'section" in lower_line: continue
                print(line)
            violations += 1
        elif args.verbose:
            print(f"✅ {rel_path} OK")

    print(f"\n--- Audit complete. {total_files} files checked, {violations} violations found. ---")
    if violations > 0:
        sys.exit(1)
    sys.exit(0)

if __name__ == "__main__":
    main()
