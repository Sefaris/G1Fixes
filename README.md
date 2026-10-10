# G1Fixes

A plugin for Gothic 1 with the following fixes and improvements:

- Fixes the trampoline bug, where jumping over holes in the world mesh could launch the player into the sky.
- Removes jumping from the right mouse button.
- Allows mouse buttons to be bound in game and plugin settings.
- Adds missing English names for mouse controls.
- Optionally provides Polish names for mouse controls and selected keyboard keys.

## Configuration

Add or update the following section in `Gothic.ini`:

```ini
[G1FIXES]
; 0 - Keep existing keyboard names and use English names for added mouse controls
; 1 - Use Polish names for mouse controls and selected keyboard keys
; Default when unset: 1 if the detected system language is Polish, otherwise 0
PolishInputNames=1
```

## Installation

Download `G1Fixes.vdf` and copy it to the `Data/Plugins` folder in your Gothic 1 installation.
