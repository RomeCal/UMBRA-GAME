# Did UMBRA follow the zip's instructions? (honest audit)

Legend: **DONE** = in the code/files and checked as far as possible without a GPU · **PARTLY** · **YOURS** = only the group can do it

## Part 1 — the game (B1–B14)
| # | Requirement | Status | Where |
|---|---|---|---|
| B1 | GL 4.6, `#version 460` | DONE (falls back to 4.1 on macOS / 3.3, logged) | `Window.cpp`, every `Shaders/*` |
| B2 | every draw `glDrawElements` | DONE (including the 1-point probe and the HUD) | `Mesh.cpp`, `SunProbe.cpp`, `HUD.cpp` |
| B3 | resize-safe projection | DONE | `main.cpp` (projection rebuilt each frame) |
| B4 | deltaTime movement | DONE (+ dt clamp) | `Game.cpp`, `Camera.cpp` |
| B5 | ≥5 materials | DONE (10) | `Scenes/*`, `docs/T07` |
| B6 | Phong, moving eye | DONE | `shader.frag` |
| B7 | sun + shadow map | DONE | `shader.frag`, `DirectionalLight.cpp` |
| B8 | two point lights, shadows | DONE (cube maps, 1 pass/light) | `omni_shadow_map.*` |
| B9 | spot light, visible cone | DONE (unshadowed — decision #4) | `shader.frag` |
| B10 | skybox | DONE (six face files + generated fallback) | `Skybox.cpp`, `Textures/sky_*.tga` |
| B11 | scene from a text file | DONE (lights, bias, box, materials, textures, paths, goal, health, sky) | `SceneLoader.cpp` |
| B12 | HUD, orthographic | DONE | `HUD.cpp`, `hud.*` |
| B13 | win/lose/restart | DONE | `Game.cpp` |
| B14 | no absolute paths | DONE (`FindAsset`) | `CommonValues.h` |
| — | the mechanic (shadows are the game), two routes, movable geometry | DONE, routes verified by simulation | `SunProbe`, `check_levels.py` |

## Part 2 — proofs: the tools exist (keys 1/2/3, Z/X, N/M, F12, G), the **measurements are YOURS**.
## Part 3 — documents: drafts are in `docs/`; measured tables (T11, T12, T13), signatures, clean-clone records (T05), weekly logs (T01), peer form, video: **YOURS**.
## Part 4 — report S1–S5: material is in `docs/S1_frame_draft.txt`, `S2_notes.txt`; **writing it is YOURS**.
## Part 5 — demo: `docs/DEMO_SCRIPT.txt` (break-it keys included); **rehearsing is YOURS**.

## Things the first version missed, now fixed
* textures/sky were generated in code only → now real files named in the scene file;
* no screenshot key / six-direction look / shadows-off demo key → added;
* shadow box did not cover the arena (a real bug) → sizes fixed, checker added;
* level 1 had no real short-vs-long route choice → redesigned;
* the bias derivation, texture-unit ledger, decision log (with the AI-code declaration) etc. did not exist → drafted in `docs/`.

## Things nobody can promise
* that it compiles first try on your machine, or its frame rate (no GPU was available);
* that the difficulty feels right (numbers come from simulation, not play-testing).
