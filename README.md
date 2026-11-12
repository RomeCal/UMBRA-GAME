
The idea
UMBRA is a first-person survival puzzle where shadow is the only safe ground. You’re in a sun-baked courtyard. Standing in direct sunlight drains your health and standing in shade restores it. You win by reaching the glowing blue pillar.

The sun and shade aren’t scripted zones. Each frame, a tiny GPU pass samples the sun’s real shadow map at five points around your feet and reports how much of you is lit. That number drives your health. Anything that casts a shadow in the picture is real cover in the game. If the shadow rendering is wrong, the game is wrong, which is why the bias, shadow-box and shadows-off keys work as demos.

Moment to moment
Health: it drains at a level-specific rate when fully lit (30/s in level 1) and regenerates at 12/s in full shade. Half-lit means a blend of the two.
Stamina: Shift sprints at 1.7× speed and drains stamina. When it hits zero you’re TIRED until 30% is back, so you can’t sprint across the sun forever.
Pushable blocks: the orange crates move when you walk into them. A block casts shade wherever you leave it, so you can build your own cover.
Collision: you collide with the same object list that is drawn and shadowed.
Feedback: a red-orange flash while you burn, a pulsing red frame when health is low, and a SUN/SHADE indicator.
HUD: health and stamina bars (top left), level, time and distance to the goal (top right), a Tab minimap (bottom right) and wrapped tips (bottom, left of the map).
Scoring: three stars if you finish under the level’s par time, two under 1.6× par, otherwise one. Best times are kept per level for the session.
The three levels
1 First Light	2 Swinging Sun	3 The Long Noon
Sun	fixed	swings ±12° every 40 s	slanted, swings ±8° every 36 s
Arena	42 × 28	42 × 28 (mirrored)	52 × 30
Drain / regen	30 / 12	34 / 12	40 / 12
Par time	60 s	55 s	75 s
Idea	learn the mechanic	shade edges move, stay deep inside it	a gap in the shade the crates must close
Level 1:
Walls zig-zag up the west edge, then run east along the north.
There’s a doorway in the north wall.
The safe route is about 61 units long and the risky shortcut across the sand is about 47.
Level 2: it’s level 1 mirrored, so you start east and finish west, with taller walls (9 high). Because the sun swings, the safe shade moves every few seconds.
Level 3:
Sandstone walls, obsidian pillars and a different floor.
The sun leans east, so shadows slant north-east.
The north wall has a wide lit gap, and two tall crates are meant to be pushed into it.
In my simulation the shortcut is lethal, which is the intent.
What you’ll see
Lighting: one sun with a 2048² shadow map, two torches that cast cube-map shadows in all directions, and two spot lights (your flashlight on T, plus a fixed one).
Surfaces: Phong shading (ambient, diffuse, specular). There are ten materials, from matt sand to a sharp-glinting metal pyramid and glowing ember and goal materials.
Sky and atmosphere: the sky is a cube map from six face files, with a sun disc that follows the real light direction, plus distance haze.
Assets: ten textures from Textures/, plus four generated shapes (box, plane, cylinder, pyramid).
Scene files: everything (lights, health numbers, shadow box, materials, goal, tips) comes from Scenes/*.txt, so you can edit and re-run without rebuilding. A bad line prints its line number and the game keeps running.
Controls
Key	What it does
WASD, mouse, Shift	move, look, sprint
Tab / R / P or Esc	minimap / restart / pause
T / V	flashlight / vsync
F1	debug panel (timings, bias, box size, texel size)
F2	free camera with no damage
F3	shadow edge filtering: off, 3×3, 5×5
1 / 2 / 3	bias too small (acne) / about right / too large (peter-panning)
0	sun shadows off; the whole level becomes lethal
Z / X	halve or double the shadow-box half-size
N / M	box follows you / texel snapping on or off
G	redraw shadow maps every frame vs only when something changed
C (in free camera)	look along +X, −X, +Y, −Y, +Z, −Z for the sky seam proof
F12 or K	screenshot to screenshots/
