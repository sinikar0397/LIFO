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

**GUI build:** needs SDL2 dev libs which are NOT installed (only runtime DLLs are in `build/`). Install: `pacman -S mingw-w64-ucrt-x86_64-SDL2 SDL2_image SDL2_ttf`. Prebuilt `build/main.exe` already exists and runs directly (DLLs are alongside it).

Note: the Bash tool here runs MSYS bash (not PowerShell despite env banner). Use forward-slash paths like `/c/Users/sungw/OneDrive/문서/GitHub/LIFO`; backslashes get eaten as escapes. PowerShell cmdlets (Select-Object, Get-Content) get blocked by the permission classifier — use bash equivalents (head, cat). See [[project-overview]].
