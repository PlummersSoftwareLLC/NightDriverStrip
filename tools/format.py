#!/usr/bin/env python3
"""
Wrapper script that runs uncrustify followed by preprocessor indentation fixes.

This script provides a seamless formatting experience by:
1. Running uncrustify with the project configuration
2. Automatically fixing preprocessor directive indentation
3. Supporting all uncrustify command-line options

Usage:
    format.py [uncrustify options] <files...>
    
Examples:
    format.py --replace --no-backup src/effects.cpp
    format.py -c custom.cfg --replace src/*.cpp
"""

import os
import sys
import subprocess
from pathlib import Path

def find_project_root() -> Path:
    """Find the project root directory containing uncrustify.cfg."""
    current = Path.cwd()
    
    # Look for uncrustify.cfg in current directory and parent directories
    for path in [current] + list(current.parents):
        config_file = path / "uncrustify.cfg"
        if config_file.exists():
            return path
    
    # Fall back to current directory if no config found
    return current

def run_uncrustify(args: list) -> int:
    """
    Run uncrustify with the provided arguments.
    
    Args:
        args: Command line arguments for uncrustify
        
    Returns:
        Exit code from uncrustify
    """
    cmd = ["uncrustify"] + args
    try:
        result = subprocess.run(cmd, check=False, capture_output=False)
        return result.returncode
    except FileNotFoundError:
        print("Error: uncrustify not found. Please install uncrustify.", file=sys.stderr)
        return 1

def run_pp_fix(file_path: str) -> int:
    """
    Run the preprocessor indentation fix on a file.
    
    Args:
        file_path: Path to the file to fix
        
    Returns:
        Exit code (0 for success)
    """
    script_dir = Path(__file__).parent
    fix_script = script_dir / "fix_pp_simple.py"
    
    if not fix_script.exists():
        print(f"Warning: {fix_script} not found, skipping preprocessor fix", file=sys.stderr)
        return 0
    
    cmd = [sys.executable, str(fix_script), file_path]
    try:
        result = subprocess.run(cmd, check=False, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout, end='')
        if result.stderr:
            print(result.stderr, end='', file=sys.stderr)
        return result.returncode
    except Exception as e:
        print(f"Error running preprocessor fix: {e}", file=sys.stderr)
        return 1

def extract_files_from_args(args: list) -> list:
    """
    Extract file paths from uncrustify arguments.
    
    Args:
        args: Uncrustify command line arguments
        
    Returns:
        List of file paths that will be modified
    """
    files = []
    skip_next = False
    
    for i, arg in enumerate(args):
        if skip_next:
            skip_next = False
            continue
            
        # Skip options that take parameters
        if arg in ['-c', '--config', '-o', '--output', '-l', '--language']:
            skip_next = True
            continue
            
        # Skip other options
        if arg.startswith('-'):
            continue
            
        # This should be a file
        if os.path.exists(arg):
            files.append(arg)
    
    return files

def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage: format.py [uncrustify options] <files...>")
        print("\nThis wrapper runs uncrustify followed by preprocessor indentation fixes.")
        print("\nExamples:")
        print("  format.py --replace --no-backup src/effects.cpp")
        print("  format.py -c custom.cfg --replace src/*.cpp")
        sys.exit(1)
    
    # Get uncrustify arguments (everything except our script name)
    uncrustify_args = sys.argv[1:]
    
    # Add default config if none specified
    has_config = any(arg in ['-c', '--config'] for arg in uncrustify_args)
    if not has_config:
        project_root = find_project_root()
        config_file = project_root / "uncrustify.cfg"
        if config_file.exists():
            # Insert config option at the beginning
            uncrustify_args = ['-c', str(config_file)] + uncrustify_args
            print(f"Using config: {config_file}")
    
    # Run uncrustify
    print("Running uncrustify...")
    exit_code = run_uncrustify(uncrustify_args)
    
    if exit_code != 0:
        print(f"uncrustify failed with exit code {exit_code}", file=sys.stderr)
        sys.exit(exit_code)
    
    # Extract files that were modified (only if --replace or similar is used)
    if '--replace' in uncrustify_args or '--no-backup' in uncrustify_args:
        files_to_fix = extract_files_from_args(uncrustify_args)
        
        if files_to_fix:
            print("Fixing preprocessor indentation...")
            for file_path in files_to_fix:
                pp_exit_code = run_pp_fix(file_path)
                if pp_exit_code != 0:
                    print(f"Preprocessor fix failed for {file_path}", file=sys.stderr)
                    # Don't exit, continue with other files
    
    print("Formatting complete!")

if __name__ == "__main__":
    main()