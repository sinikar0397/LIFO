---
name: build-and-run
description: How to build/run the LIFO C project (no Makefile) and the PATH gotcha that makes gcc fail silently
metadata:
  type: project
---

LIFO has no Makefile; build manually with MSYS2 UCRT64 gcc. Two entry points: `main.c` (console login test) and `main_ui.c` (SDL2 GUI).

**PATH gotcha (critical):** `gcc` is on PATH but `cc1.exe`'s dependent DLLs live in `/c/msys64/ucrt64/bin`. If that dir is NOT on PATH, gcc fails with NO error output (cc1 crashes on launch) and just returns exit 1. Always prefix builds with:
`export PATH="/c/msys64/ucrt64/bin:$PATH"`

**Console build (works, no external deps):**
`gcc main.c src/cJSON.c src/people/login.c src/people/people.c -I src -o build/console.exe`

**Build scripts (canonical, added 2026-06-11):** `build.sh` (MSYS2 bash; `./build.sh` = GUI, `./build.sh console` = console) handles the PATH export internally. `run.ps1` (PowerShell; `.\run.ps1` builds via bash then runs the exe). Don't run run.ps1 through the Bash tool — it's PowerShell-only and errors; call `./build.sh` directly there instead.

**GUI build (works as of 2026-06-09):** SDL2 dev libs installed (`mingw-w64-ucrt-x86_64-SDL2 SDL2_image SDL2_ttf`). Build with:
`gcc main_ui.c src/cJSON.c src/people/login.c src/people/people.c src/gui/ui.c src/gui/display.c src/dfs/dfs.c $(pkg-config --cflags sdl2 SDL2_image SDL2_ttf) $(pkg-config --libs sdl2 SDL2_image SDL2_ttf) -I src -o build/main.exe`
(`src/dfs/dfs.c` was added to the GUI build 2026-06-11 when the DFS survey screen was wired in.) Run `./build/main.exe` (runtime DLLs + gulsi.ttf are in `build/`). Don't run it from the Bash tool — it opens a window and blocks.

Note: the Bash tool here runs MSYS bash (not PowerShell despite env banner). Use forward-slash paths like `/c/Users/sungw/OneDrive/문서/GitHub/LIFO`; backslashes get eaten as escapes. PowerShell cmdlets (Select-Object, Get-Content) get blocked by the permission classifier — use bash equivalents (head, cat). See [[project-overview]].
