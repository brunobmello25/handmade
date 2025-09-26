#!/usr/bin/env python3
import os
import json
import glob
import re

def extract_defines_and_typedefs(file_path):
    """Extract #defines and typedefs from a C/C++ file"""
    defines = []
    includes = []
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
        
        # Extract #define statements (simple ones)
        define_pattern = r'#define\s+(\w+)\s+([^\n]+)'
        for match in re.finditer(define_pattern, content):
            name, value = match.groups()
            # Skip function-like macros and complex defines
            if '(' not in name and not value.strip().startswith('('):
                defines.append(f"-D{name}={value.strip()}")
        
        # Extract typedef statements
        typedef_pattern = r'typedef\s+(\w+(?:\s*\*)*)\s+(\w+);'
        for match in re.finditer(typedef_pattern, content):
            old_type, new_type = match.groups()
            defines.append(f"-D{new_type}={old_type.strip()}")
        
        # Extract #include statements for system headers
        include_pattern = r'#include\s+<([^>]+)>'
        for match in re.finditer(include_pattern, content):
            header = match.group(1)
            # Only include standard C/C++ headers
            if header in ['stdint.h', 'math.h', 'stdio.h', 'stdlib.h', 'string.h', 'stdbool.h']:
                includes.append(f"-include {header}")
        
    except Exception as e:
        print(f"Warning: Could not parse {file_path}: {e}")
    
    return defines, includes

def find_active_platform_file(bear_commands):
    """Find the platform file that was actually compiled by bear"""
    for cmd in bear_commands:
        file_path = cmd.get("file", "")
        # Look for the main compiled file (not the ones added by our script)
        if file_path.endswith(".cpp") and "code/" in file_path:
            # Check if this file includes handmade.cpp
            try:
                with open(file_path, 'r') as f:
                    content = f.read()
                if '#include "handmade.cpp"' in content:
                    return file_path
            except:
                continue
    return None

def extend_compile_commands():
    compile_commands_path = "target/compile_commands.json"
    
    if not os.path.exists(compile_commands_path):
        print("No compile_commands.json found in target/")
        return
    
    with open(compile_commands_path, "r") as f:
        bear_commands = json.load(f)
    
    if not bear_commands:
        print("Empty compile_commands.json")
        return
    
    # Start with the bear commands
    extended_commands = list(bear_commands)
    
    # Find the active platform file from bear output
    platform_file = find_active_platform_file(bear_commands)
    if not platform_file:
        print("Warning: Could not find active platform file")
        platform_defines = []
        platform_includes = []
        platform_flags = []
    else:
        print(f"Found active platform file: {platform_file}")
        platform_defines, platform_includes = extract_defines_and_typedefs(platform_file)
        
        # Find the compilation flags for the platform file
        platform_flags = []
        for cmd in bear_commands:
            if cmd.get("file") == platform_file:
                if "arguments" in cmd:
                    # Extract flags from arguments (skip compiler, -c, -o, output file, input file)
                    args = cmd["arguments"]
                    for arg in args:
                        if arg.startswith("-I") or arg.startswith("-D"):
                            platform_flags.append(arg)
                elif "command" in cmd:
                    # Extract flags from command string
                    import shlex
                    args = shlex.split(cmd["command"])
                    for arg in args:
                        if arg.startswith("-I") or arg.startswith("-D"):
                            platform_flags.append(arg)
                break
    
    # Find all .cpp and .h files and add entries for clangd
    for pattern in ["code/*.cpp", "code/*.h"]:
        for file_path in glob.glob(pattern):
            full_path = os.path.join(os.getcwd(), file_path)
            # Skip if already in bear output
            if any(cmd["file"] == full_path for cmd in bear_commands):
                continue
            
            # For handmade.cpp and handmade.h, use the platform compilation context
            if file_path in ["code/handmade.cpp", "code/handmade.h"]:
                # Build the command with platform flags and extracted definitions
                cmd_parts = ["c++", "-c", "-g"]
                cmd_parts.extend(platform_flags)
                cmd_parts.extend(platform_includes)
                cmd_parts.extend(platform_defines)
                cmd_parts.extend(["-o", "handmade", file_path])
                
                extended_commands.append({
                    "directory": os.getcwd(),
                    "command": " ".join(cmd_parts),
                    "file": full_path,
                    "output": os.path.join(os.getcwd(), "handmade")
                })
                
                print(f"Added {file_path} with {len(platform_defines)} defines and {len(platform_flags)} flags")
            else:
                # For other files, create a basic compilation command
                extended_commands.append({
                    "directory": os.getcwd(),
                    "command": f"c++ -c -g -o handmade {file_path}",
                    "file": full_path,
                    "output": os.path.join(os.getcwd(), "handmade")
                })
    
    with open(compile_commands_path, "w") as f:
        json.dump(extended_commands, f, indent=2)
    
    print(f"Extended compile_commands.json to {len(extended_commands)} entries")

if __name__ == "__main__":
    extend_compile_commands()