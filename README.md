# UMBRA v2 — Shadow is solid ground (Group 3)

First-person survival puzzle. **The sun burns you; shade heals you.** Reach the blue light.
"In the sun" is decided by the sun's own shadow map, sampled on the GPU at your feet (`SunProbe` + `sunprobe.frag`) — not by trigger zones.
Three levels: **First Light** (static sun) → **Swinging Sun** (the sun swings ±12°) → **The Long Noon** (slanted sun, a gap in the shade the crates must close).

## macOS (Terminal)
```
xcode-select --install            # skip if you already have the compiler
brew install glew glfw glm        # Homebrew: https://brew.sh
cd ~/Downloads/UMBRA_Mac
chmod +x build_and_run.sh
./build_and_run.sh                # level 1.   Other levels:  ./build_and_run.sh Scenes/level3.txt
```
macOS only supports OpenGL up to 4.1, so the program asks for 4.6, then 4.1, then 3.3 and rewrites each shader's `#version` line to match (it prints which one it got).

## Windows (Visual Studio 2022 Community)
```
vcpkg install glew:x64-windows glfw3:x64-windows glm:x64-windows
vcpkg integrate install
```
Open `UMBRA.sln`, choose **x64**, press **F5**. (VS 2019: Project → Properties → General → Platform Toolset = v142.)
Alternative: *File → Open → Folder* on this folder (CMake, needs the vcpkg toolchain).
The program finds `Shaders/`, `Scenes/`, `Textures/` from the project folder or from `x64/Debug` — no absolute paths.

## Controls
| | |
|---|---|
| **WASD** move · **mouse** look · **Shift** run (uses stamina) · **Tab** map | push the **orange blocks** to make new shade |
| **Enter** start/continue · **R** restart · **P / Esc** pause | **T** flashlight · **V** vsync · **F12 or K** screenshot (BMP in `screenshots/`) |
| **F1** debug panel · **F2** free camera · **F3** PCF off/3×3/5×5 | **C** (free cam) look along +X −X +Y −Y +Z −Z |
| **1 / 2 / 3** bias too small / right / too large | **Z / X** halve / double shadow-box half-size |
| **N** box follows player · **M** texel snapping | **G** redraw shadows every frame vs only when needed · **0** shadows off |

## What's new in v2
* Real asset files: 10 textures + 6 sky face files in `Textures/`, all named in the scene files (`tools/make_assets.py` regenerates them).
* Level 3, a new level; levels verified by simulation (`python3 tools/check_levels.py Scenes/level1.txt`): both routes, shade coverage, **and the shadow box covers the whole arena** (this caught a real bug: with half-size 20 the arena edges were outside the box and burned you).
* Gameplay: stamina, minimap, tips, three-star rating, best times, low-health warning, fog; text drop shadows.
* Proof tools: screenshot key, six-direction camera, shadows-off key, GPU/CPU/wall timing, scene errors with line numbers.
* `docs/`: the group deliverables (T2–T14, scene format, bias derivation, S1/S2 notes, demo script). Start with `docs/00_START_HERE_FOR_THE_GROUP.txt`.

## Honest status
Written and type-checked, **never run on a GPU** by the author. The loader, the level checker and the file generators were run; rendering was not.
If something fails to compile or looks wrong, the console message plus your compiler output is all that's needed to fix it.
