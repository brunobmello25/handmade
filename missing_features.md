# Missing Features in SDL3 Platform Layer

Comparison between SDL3 platform layer and Casey's Win32 platform layer (Episode 25).

## Missing Features

### 1. **Mouse Input**

- Win32 has complete mouse support: position (MouseX, MouseY, MouseZ), and 5 mouse buttons
- SDL3: No mouse input handling at all

### 2. **Separate Sound Callback Function**

- Win32 has `GameGetSoundSamples` as a separate function from `GameUpdateAndRender`
- SDL3: Only has `gameUpdateAndRender` which handles both rendering and sound inline

### 3. **Thread Context Parameter**

- Win32 passes a `thread_context *Thread` parameter to game functions
- SDL3: No thread context support (your game functions don't accept this parameter)

### 4. **File-Mapped Memory for Replay Buffers**

- Win32 uses `CreateFileMapping` and `MapViewOfFile` to map replay buffers to files (4 replay buffers at code-tutorial/win32_handmade.cpp:1106-1138)
- SDL3: Replay buffers only exist in-memory, not file-mapped

### 5. **Smart Frame Timing with Sleep Granularity**

- Win32 uses `timeBeginPeriod(1)` to improve sleep granularity (line 1001), then uses both `Sleep()` and busy-waiting for precise frame timing (lines 1479-1500)
- SDL3: Only uses `SDL_Delay()` which may be less precise

### 6. **Dynamic Game Update Rate**

- Win32 runs at half the monitor refresh rate (`GameUpdateHz = MonitorRefreshHz / 2.0f` at line 1044)
- SDL3: Runs at the full monitor refresh rate

### 7. **Sophisticated Audio Latency Compensation**

- Win32 has complex audio sync logic (lines 1353-1465): calculates expected frame boundary, has safety bytes, handles low-latency vs high-latency audio cards, predicts where the play cursor will be
- SDL3: Simpler audio queuing - just checks if queue is below target and generates audio based on time elapsed

### 8. **Hot-Reload DLL Change Detection**

- Win32 checks file modification time (`Win32GetLastWriteTime`, `CompareFileTime`) every frame to auto-reload DLL when changed (lines 1163-1170)
- SDL3: Unloads and loads every single frame unconditionally (lines 749-750)

### 9. **Temporary DLL Copy for Loading**

- Win32 copies `handmade.dll` → `handmade_temp.dll` before loading to allow the original to be overwritten (line 240)
- SDL3: Loads the DLL directly without copying

### 10. **Pause Functionality**

- Win32 has a `GlobalPause` flag toggled with 'P' key that can pause the game update loop (lines 809-814, 1189)
- SDL3: No pause feature

### 11. **EXE Path Management**

- Win32 gets its own executable path (`Win32GetEXEFileName`) and builds file paths relative to the EXE (lines 95-130)
- SDL3: Uses hardcoded relative paths for snapshots and library

### 12. **Shoulder Button Keyboard Mapping**

- Win32 maps Q/E keys to left/right shoulder buttons (lines 776-782)
- SDL3: No keyboard mapping for shoulder buttons

### 13. **Arrow Keys and Space/Escape Mapping**

- Win32 maps arrow keys to action buttons, Escape to Start, Space to Back (lines 784-806)
- SDL3: Only ESC exits the game, no other special key mappings

### 14. **Alt+F4 Handling**

- Win32 explicitly handles Alt+F4 to close (lines 841-845)
- SDL3: Doesn't handle this (though SDL might handle it automatically)

### 15. **Multiple Replay Slot Support (Infrastructure)**

- Win32 has full infrastructure for 4 different replay slots (array of replay buffers)
- SDL3: Only supports one replay slot (hardcoded slot 1)

## Notes on Equivalent or Different Approaches

- **Hot Reloading**: Your approach (auto-build on save + load every frame) works differently but achieves the same goal
- **Transient Memory Snapshotting**: You intentionally skip this for performance reasons
- **Controller Deadzone**: You have a TODO but Win32 implements it (line 618-631 in win32_handmade.cpp)
- **VSync**: Your SDL3 version enables it, Win32 doesn't mention it explicitly

## Most Critical Gaps

The most significant functional gaps are:

1. **Mouse input** - affects the game API contract
2. **Separate sound generation function** - affects the game API contract
3. **Thread context parameter** - affects the game API contract

These three require changes to the game layer interface to match Win32's API.
