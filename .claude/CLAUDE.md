# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Is

PianoCheetah is a Qt6/C++17 MIDI sequencer and piano-learning application ("pianohero++"), deployed as a Flatpak on KDE Plasma 6.10. It includes a main GUI app, a MIDI device config tool (`midicfg`), and several CLI utilities for converting and exporting song files.

## Build

```bash
./b.sh          # build and install release flatpak
./b.sh d        # build debug flatpak (launches with gdb)
./b.sh c        # build + reset all config files to factory defaults
```

The build script uninstalls the old flatpak, wipes `_build/` and `.flatpak-builder/`, then runs `flatpak-builder`. There is no incremental build — each run is a full rebuild.

**Prerequisites:**
```bash
sudo apt install flatpak flatpak-builder php-cli
flatpak install org.kde.Sdk//6.10 org.kde.Platform//6.10
```

Note: `b.sh` is a PHP script (shebang `#!/bin/php`), not a shell script.


## Architecture

### Repo Layout

- `pianocheetah/` — main Qt GUI application
- `midicfg/` — MIDI device configuration editor (separate Qt app)
- `mid2song/`, `txt2song/`, `mod2song/` — import converters
- `song2wav/`, `sfz2syn/`, `synsnd/` — export/synthesis tools
- `../stv/` — shared C++ library (sibling directory, not inside this repo)

### stv Library (`../stv/`)

All apps depend on this shared library. Key modules:
- `os.{h,cpp}` — string types, file I/O, OS utilities
- `ui.{h,cpp}` — reusable Qt widgets
- `midi.{h,cpp}` — ALSA MIDI I/O
- `syn.{h,cpp}` — software synthesizer
- `snd.{h,cpp}` — audio/sound management
- `wav.{h,cpp}` — WAV file I/O

### Main App (`pianocheetah/`)

Two-thread model:
- **Main thread**: `PCheetah` (QMainWindow) owns the GUI
- **Worker thread**: `Song` (QThread) owns all song data and handles MIDI I/O, playback, and synthesis

Communication is strictly via Qt signals:
- `sgCmd(QString)` — GUI sends commands to Song thread
- `sgUpd(QString)` — Song thread sends redraw/update notifications to GUI

Song data lives in `SongFile _f` inside `Song`. The `sXxx.cpp` files are all part of `Song`'s implementation, split by concern:
- `sFile.cpp` — load/save `.song` binary files
- `sNote.cpp` — notation drawing
- `sEdit.cpp`, `sEdMs.cpp` — note editing and mouse handling
- `sRecord.cpp` — MIDI recording
- `sReDo.cpp` — undo/redo and redraw coordination
- `sCmd.cpp` — command dispatch
- `sTime.cpp` — tempo and time signatures
- `sDevice.cpp`, `sDevTyp.cpp` — MIDI device management

### Song File Format (`.song`)

Binary format with tagged sections:
- `TB_DSC` — description/metadata (tempo, transposition)
- `TB_TRK` — tracks (device, sound, channel, name)
- `TB_EVT` — MIDI events (note on/off, controllers)
- `TB_DRM` — drum map
- `TB_LYR` — lyrics
- `TB_CUE` — cue points

## Code Conventions

These extend the rules in `~/.claude/CLAUDE.md`:

**Types (from `stv/os.h`):**
- `ubyte`, `ubyt2`, `ubyt4`, `ubyt8` — unsigned 8/16/32/64-bit
- `sbyte`, `sbyt2`, `sbyt4` — signed equivalents
- `real` — `double` (never use `float`)
- `TStr` — temp path string (~240 chars)
- `WStr` — word string (32 chars)
- `BStr` — big string (10KB)

**Sentinel values:**
- `NONE` = `0x7FFFFFFF` — "no time" / absent value
- `SND_NONE` = `0xFFFFFFFF` — no sound assigned

**Learning modes** (stored as char): `LHEAR`=`'a'`, `LHREC`=`'b'`, `LPRAC`=`'c'`, `LPLAY`=`'d'`

**Duration unit:** `M_WHOLE` = one whole note in MIDI ticks
