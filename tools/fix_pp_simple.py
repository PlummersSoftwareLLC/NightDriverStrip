#!/usr/bin/env python3
"""
Very simple preprocessor indentation fixer.

This script ensures:
1. All #if/#endif blocks at the same nesting level have the same indentation as surrounding code
2. Content inside blocks is indented 4 spaces deeper than the directive
3. RegisterAll calls are indented deeper than regular content
4. Effect<> calls are indented deeper than RegisterAll
"""

import re
import sys

def fix_preprocessor_indentation(content: str) -> str:
    """Fix preprocessor indentation with a simple, reliable approach."""
    lines = content.split('\n')
    result = []
    
    for line in lines:
        stripped = line.lstrip()
        
        if not stripped:
            result.append(line)
            continue
            
        if stripped.startswith('#'):
            # This is a preprocessor directive
            pp_match = re.match(r'#\s*(if|ifdef|ifndef|else|elif|endif)', stripped)
            if pp_match:
                # For all preprocessor directives, use 4 spaces base indentation
                # (This matches the surrounding function-level code)
                fixed_line = '    ' + stripped
                result.append(fixed_line)
            else:
                # Other preprocessor directives (#define, #include, etc.)
                result.append(line)
        else:
            # Regular code line
            # Check if we're likely inside a preprocessor block by looking at context
            if is_likely_in_pp_block(result):
                # Content should be indented to 8 spaces (4 for function + 4 for PP block)
                # But function calls need deeper indentation
                if stripped.startswith('RegisterAll'):
                    # RegisterAll should be indented to 16 spaces (8 + 8 more for the function call)
                    fixed_line = '                ' + stripped  # 16 spaces
                    result.append(fixed_line)
                elif stripped.startswith('Effect<'):
                    # Effect<> calls should be indented to 20 spaces (16 + 4 more than RegisterAll)
                    fixed_line = '                    ' + stripped  # 20 spaces
                    result.append(fixed_line)
                elif stripped == ');':
                    # Closing parenthesis aligns with RegisterAll
                    fixed_line = '                ' + stripped  # 16 spaces
                    result.append(fixed_line)
                elif len(line) - len(stripped) < 8:
                    # Regular content needs 8 spaces
                    fixed_line = '        ' + stripped
                    result.append(fixed_line)
                else:
                    # Already properly indented or more
                    result.append(line)
            else:
                # Not in a PP block, preserve original
                result.append(line)
    
    return '\n'.join(result)

def is_likely_in_pp_block(preceding_lines: list) -> bool:
    """
    Check if we're likely inside a preprocessor block by counting
    #if vs #endif in recent lines.
    """
    if not preceding_lines:
        return False
        
    # Look at the last few lines to see if we're in a block
    recent_lines = preceding_lines[-20:] if len(preceding_lines) > 20 else preceding_lines
    
    if_count = 0
    endif_count = 0
    
    for line in recent_lines:
        stripped = line.lstrip()
        if re.match(r'#\s*(if|ifdef|ifndef)', stripped):
            if_count += 1
        elif re.match(r'#\s*endif', stripped):
            endif_count += 1
    
    return if_count > endif_count

def main():
    """Main entry point."""
    if len(sys.argv) != 2:
        print("Usage: fix_pp_simple.py <file_path>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        fixed_content = fix_preprocessor_indentation(content)
        
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(fixed_content)
            
        print(f"Fixed preprocessor indentation in: {file_path}")
        
    except Exception as e:
        print(f"Error processing {file_path}: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
