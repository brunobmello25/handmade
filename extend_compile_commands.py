#!/usr/bin/env python3
import os
import json
import glob

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
    
    # Find all .cpp and .h files and add entries for clangd
    for pattern in ["code/*.cpp", "code/*.h"]:
        for file_path in glob.glob(pattern):
            full_path = os.path.join(os.getcwd(), file_path)
            # Skip if already in bear output
            if any(cmd["file"] == full_path for cmd in bear_commands):
                continue
            
            # Extract filename without extension for the guard
            filename = os.path.splitext(os.path.basename(file_path))[0]
            
            extended_commands.append({
                "directory": os.getcwd(),
                "command": f"c++ -c -g -o handmade {file_path} -include code/raylib_linux_handmade.cpp",
                "file": full_path,
                "output": os.path.join(os.getcwd(), "handmade")
            })
    
    with open(compile_commands_path, "w") as f:
        json.dump(extended_commands, f, indent=2)
    
    print(f"Extended compile_commands.json to {len(extended_commands)} entries")

if __name__ == "__main__":
    extend_compile_commands()