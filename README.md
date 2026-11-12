# UMBRA — Shadow Is Solid Ground

**Group 3 · Computer Graphics Programming · OpenGL (C++)**

UMBRA is a first-person survival puzzle. The sun burns you and shade heals you. Reach the blue light before your health runs out.

The sun is not a scripted zone. Every frame the game samples the sun's real shadow map at your feet on the GPU, and the result drives your health. If a shadow is drawn, it is real cover, and if the shadow rendering is wrong, the game is wrong.

---

## Features
- Directional sun with a shadow map (orthographic shadow box, bias, optional PCF)
- Two shadow-casting point lights (cube-map shadows, one pass per light using a geometry shader)
- Spot lights with a soft cone edge, including a flashlight
- Phong lighting with 10 materials, using texture files from `Textures/`
- Skybox cube map from six face files, with a sun disc that follows the real light direction
- Orthographic HUD: health and stamina bars, SUN/SHADE indicator, timer, goal distance, minimap
- Whole levels loaded from text files (`Scenes/*.txt`) with line-numbered error messages
- Pushable blocks that cast shade where you leave them
- Three levels, three-star ratings and session best times

## Requirements
- A GPU and driver supporting OpenGL 3.3 or newer (the game asks for 4.6, then 4.1, then 3.3)
- C++14 compiler
- GLEW, GLFW 3, GLM

## Build and run
### Windows (Visual Studio 2022 Community)
```
vcpkg install glew:x64-windows glfw3:x64-windows glm:x64-windows
vcpkg integrate install
```
Open `UMBRA.sln`, set the platform to **x64**, press **F5**.
(Visual Studio 2019: Project → Properties → General → Platform Toolset = v142.)

Run the program from the project folder, or from `x64/Debug`. It finds `Shaders/`, `Scenes/` and `Textures/` automatically, with no absolute paths.

## How to play
Reach the glowing blue pillar. Stay in shadow to recover, because standing in sunlight drains health. Push the orange crates to make your own shade.

| Key | Action |
|---|---|
| W A S D / mouse | move / look |
| Shift | run (uses stamina) |
| Tab | toggle minimap |
| Enter | start / continue |
| R | restart level |
| P or Esc | pause |
| T | flashlight on/off |

### Levels
1. **First Light** — fixed sun; learn the mechanic. A short risky route and a long safe one.
2. **Swinging Sun** — the sun swings ±12°, so the shade edges move.
3. **The Long Noon** — slanted sun; a lit gap in the north shade must be closed with crates.
