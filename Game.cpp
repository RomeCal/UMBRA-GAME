#include "Game.h"

Game::Game()
    : state(GameState::TITLE), health(100.f), maxHealth(100.f), stamina(100.f), maxStamina(100.f), timer(0.f), burn(0.f),
      feetPos(0.f), freeCam(false), wantRestart(false), wantNextLevel(false), wantFirstLevel(false), wantQuit(false),
      movedObjects(false), litFraction(0.f), outsideShadowBox(false), stars(0), levelNumber(1), goal(0.f), goalRadius(1.5f),
      bounds(-20, -14, 20, 14), sunDrain(25.f), shadeRegen(12.f), walkSpeed(4.5f), sprintMul(1.6f), eyeHeight(1.7f),
      radius(0.4f), staminaDrain(40.f), staminaRegen(22.f), parTime(60.f), exhausted(false), showMap(true),
      startPos(0.f), startYaw(0.f), applyStartCamera(true) {}

void Game::LoadLevel(const SceneData& sd, const std::vector<ObjectDef>& orig, std::vector<ObjectDef>& live, int number)
{
    // Collision reads the SAME object list the renderer draws (one copy of the level, not two).
    bounds = sd.bounds; maxHealth = sd.maxHealth; sunDrain = sd.sunDrain; shadeRegen = sd.shadeRegen;
    maxStamina = sd.maxStamina; staminaDrain = sd.staminaDrain; staminaRegen = sd.staminaRegen; parTime = sd.parTime;
    walkSpeed = sd.walkSpeed; sprintMul = sd.sprintMultiplier; eyeHeight = sd.eyeHeight; radius = sd.playerRadius;
    startPos = sd.playerStart; startYaw = sd.playerYaw; goal = sd.goal; goalRadius = sd.goalRadius;
    levelName = sd.name; nextLevel = sd.nextLevel; hints = sd.hints; levelNumber = number;
    Restart(orig, live);
    state = GameState::TITLE;
}

void Game::Restart(const std::vector<ObjectDef>& orig, std::vector<ObjectDef>& live)
{
    for (size_t i = 0; i < live.size() && i < orig.size(); ++i) live[i].pos = orig[i].pos;   // blocks go back
    health = maxHealth; stamina = maxStamina; exhausted = false; timer = 0.f; burn = 0.f; freeCam = false; stars = 0;
    feetPos = glm::vec3(startPos.x, 0.f, startPos.y);
    applyStartCamera = true;
    movedObjects = true;
    state = GameState::PLAYING;
}

bool Game::BoxFree(const std::vector<ObjectDef>& objs, size_t skip, const glm::vec3& np) const
{
    const ObjectDef& me = objs[skip];
    glm::vec2 mn(np.x - me.scale.x * 0.5f, np.z - me.scale.z * 0.5f), mx(np.x + me.scale.x * 0.5f, np.z + me.scale.z * 0.5f);
    if (mn.x < bounds.x || mn.y < bounds.y || mx.x > bounds.z || mx.y > bounds.w) return false;
    for (size_t j = 0; j < objs.size(); ++j)
    {
        if (j == skip || !objs[j].solid) continue;
        const ObjectDef& o = objs[j];
        glm::vec2 omn(o.pos.x - o.scale.x * 0.5f, o.pos.z - o.scale.z * 0.5f), omx(o.pos.x + o.scale.x * 0.5f, o.pos.z + o.scale.z * 0.5f);
        if (mn.x < omx.x - 1e-3f && mx.x > omn.x + 1e-3f && mn.y < omx.y - 1e-3f && mx.y > omn.y + 1e-3f) return false;
    }
    // the player must not be squashed
    glm::vec2 c(feetPos.x, feetPos.z), cp = glm::clamp(c, mn, mx);
    if (glm::length(c - cp) < radius * 0.9f) return false;
    return true;
}

// Circle (the player) against axis-aligned boxes (every 'solid' object), in the XZ plane.
void Game::ResolveCollisions(std::vector<ObjectDef>& objs)
{
    for (int iter = 0; iter < 4; ++iter)
    {
        bool any = false;
        for (size_t i = 0; i < objs.size(); ++i)
        {
            ObjectDef& o = objs[i];
            if (!o.solid) continue;
            glm::vec2 mn(o.pos.x - o.scale.x * 0.5f, o.pos.z - o.scale.z * 0.5f), mx(o.pos.x + o.scale.x * 0.5f, o.pos.z + o.scale.z * 0.5f);
            glm::vec2 c(feetPos.x, feetPos.z), cp = glm::clamp(c, mn, mx), d = c - cp;
            float len = glm::length(d), pen; glm::vec2 n;
            if (len > 1e-5f)
            {
                if (len >= radius) continue;
                n = d / len; pen = radius - len;
            }
            else
            {   // centre ended up inside the box: leave by the nearest side
                float dl = c.x - mn.x, dr = mx.x - c.x, db = c.y - mn.y, dtp = mx.y - c.y;
                float m = std::min(std::min(dl, dr), std::min(db, dtp));
                if (m == dl) { n = glm::vec2(-1, 0); pen = dl + radius; }
                else if (m == dr) { n = glm::vec2(1, 0); pen = dr + radius; }
                else if (m == db) { n = glm::vec2(0, -1); pen = db + radius; }
                else { n = glm::vec2(0, 1); pen = dtp + radius; }
            }
            any = true;
            if (o.movable)
            {   // push the block away along its dominant axis, if there is room
                glm::vec2 push = (std::fabs(n.x) > std::fabs(n.y)) ? glm::vec2(n.x > 0 ? -1.f : 1.f, 0.f)
                                                                    : glm::vec2(0.f, n.y > 0 ? -1.f : 1.f);
                glm::vec3 np = o.pos; np.x += push.x * pen; np.z += push.y * pen;
                if (BoxFree(objs, i, np)) { o.pos = np; movedObjects = true; continue; }
            }
            feetPos.x += n.x * pen; feetPos.z += n.y * pen;
        }
        if (!any) break;
    }
}

void Game::MovePlayer(glm::vec2 delta, std::vector<ObjectDef>& objs)
{
    feetPos.x += delta.x; ResolveCollisions(objs);
    feetPos.z += delta.y; ResolveCollisions(objs);
    feetPos.x = glm::clamp(feetPos.x, bounds.x + radius, bounds.z - radius);   // cannot fall out of the level
    feetPos.z = glm::clamp(feetPos.z, bounds.y + radius, bounds.w - radius);
}

void Game::Update(Window& w, Camera& cam, std::vector<ObjectDef>& objs, float dt, float sunLit, bool outsideBox)
{
    movedObjects = false; wantRestart = wantNextLevel = wantFirstLevel = false;
    litFraction = sunLit; outsideShadowBox = outsideBox;

    if (applyStartCamera) { cam.setAngles(startYaw, -5.f); applyStartCamera = false; }
    if (w.Pressed(GLFW_KEY_TAB)) showMap = !showMap;

    // ---- state machine (docs/T10_state_machine.txt) ----
    if (state != GameState::TITLE && w.Pressed(GLFW_KEY_R)) { wantRestart = true; return; }
    switch (state)
    {
    case GameState::TITLE:
        if (w.Pressed(GLFW_KEY_ENTER) || w.Pressed(GLFW_KEY_SPACE)) state = GameState::PLAYING;
        if (w.Pressed(GLFW_KEY_ESCAPE)) wantQuit = true;
        break;
    case GameState::PAUSED:
        if (w.Pressed(GLFW_KEY_ESCAPE) || w.Pressed(GLFW_KEY_P) || w.Pressed(GLFW_KEY_ENTER)) state = GameState::PLAYING;
        break;
    case GameState::WON:
        if (w.Pressed(GLFW_KEY_ENTER)) { if (!nextLevel.empty()) wantNextLevel = true; else wantFirstLevel = true; }
        if (w.Pressed(GLFW_KEY_ESCAPE)) wantQuit = true;
        break;
    case GameState::LOST:
        if (w.Pressed(GLFW_KEY_ENTER)) wantRestart = true;
        if (w.Pressed(GLFW_KEY_ESCAPE)) wantQuit = true;
        break;
    case GameState::PLAYING:
        if (w.Pressed(GLFW_KEY_ESCAPE) || w.Pressed(GLFW_KEY_P)) { state = GameState::PAUSED; break; }
        timer += dt;

        if (!freeCam)
        {
            glm::vec3 f = cam.getFlatForward(), r = cam.getFlatRight();
            glm::vec3 wish(0.f);
            if (w.Key(GLFW_KEY_W) || w.Key(GLFW_KEY_UP))    wish += f;
            if (w.Key(GLFW_KEY_S) || w.Key(GLFW_KEY_DOWN))  wish -= f;
            if (w.Key(GLFW_KEY_D) || w.Key(GLFW_KEY_RIGHT)) wish += r;
            if (w.Key(GLFW_KEY_A) || w.Key(GLFW_KEY_LEFT))  wish -= r;
            bool moving = glm::length(wish) > 0.001f;
            bool sprinting = moving && w.Key(GLFW_KEY_LEFT_SHIFT) && stamina > 0.f && !exhausted;
            if (sprinting)
            {
                stamina -= staminaDrain * dt;
                if (stamina <= 0.f) { stamina = 0.f; exhausted = true; }   // must recover before sprinting again
            }
            else
            {
                stamina = std::min(maxStamina, stamina + staminaRegen * dt);
                if (exhausted && stamina >= maxStamina * 0.3f) exhausted = false;
            }
            if (moving)
            {
                float speed = walkSpeed * (sprinting ? sprintMul : 1.f);
                wish = glm::normalize(wish) * speed * dt;     // B4: distance = speed * deltaTime
                MovePlayer(glm::vec2(wish.x, wish.z), objs);
            }
            cam.setPosition(feetPos + glm::vec3(0.f, eyeHeight, 0.f));

            // THE MECHANIC: sunlit fraction comes from the shadow map, via the GPU probe.
            float f01 = glm::clamp(litFraction, 0.f, 1.f);
            health += (shadeRegen * (1.f - f01) - sunDrain * f01) * dt;
            health = glm::clamp(health, 0.f, maxHealth);
            burn += (f01 - burn) * std::min(1.f, dt * 6.f);

            if (health <= 0.f) state = GameState::LOST;
            else if (glm::distance(glm::vec2(feetPos.x, feetPos.z), goal) < goalRadius)
            {
                state = GameState::WON;
                stars = timer <= parTime ? 3 : (timer <= parTime * 1.6f ? 2 : 1);
                auto it = bestTimes.find(levelName);
                if (it == bestTimes.end() || timer < it->second) bestTimes[levelName] = timer;
            }
        }
        break;
    }
}

static std::string Num(float v, int decimals)
{
    char b[32]; std::snprintf(b, sizeof(b), "%.*f", decimals, v); return b;
}

// Splits text into lines no wider than maxWidth pixels (whole words only).
static std::vector<std::string> WrapText(const HUD& hud, const std::string& text, float scale, float maxWidth)
{
    std::vector<std::string> lines; std::string cur, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string trial = cur.empty() ? word : cur + " " + word;
        if (!cur.empty() && hud.TextWidth(trial, scale) > maxWidth) { lines.push_back(cur); cur = word; }
        else cur = trial;
        word.clear();
    };
    for (size_t i = 0; i <= text.size(); ++i)
    {
        if (i == text.size() || text[i] == ' ') flushWord(); else word += text[i];
    }
    if (!cur.empty()) lines.push_back(cur);
    return lines;
}

void Game::DrawHUD(HUD& hud, int w, int h, const std::vector<std::string>& debug, int warnings,
                   const std::vector<ObjectDef>& objs, const glm::vec3& camFront) const
{
    const float W = (float)w, H = (float)h;
    // One unit 's' = one font pixel.  A letter is 5s wide, 7s tall, advance 6s.  Everything below is laid out in
    // multiples of s from a margin 'm', so nothing depends on the window size except the scale itself.
    const float s = std::max(2.f, std::floor(H / 360.f));
    const float m = 6.f * s;                 // screen-edge margin
    const float th = 7.f * s;                // text height
    const float lineH = 10.f * s;            // text height + gap
    const float pad = 3.f * s;               // padding inside panels
    const glm::vec4 white(1, 1, 1, 1), panel(0.03f, 0.03f, 0.06f, 0.62f);

    auto centredAt = [&](const std::string& str, float cx, float y, float sc, glm::vec4 c) {
        hud.Text(str, cx - hud.TextWidth(str, sc) * 0.5f, y, sc, c);
    };
    auto centred = [&](const std::string& str, float y, float sc, glm::vec4 c) { centredAt(str, W * 0.5f, y, sc, c); };

    // ---- map geometry (needed first: the tips must stay clear of it) ----
    float mapW = 0.f, mapH = 0.f, mapX = 0.f, mapY = m, mapScale = 1.f;
    {
        float bw2 = bounds.z - bounds.x, bh2 = bounds.w - bounds.y;
        mapScale = std::min(W * 0.22f / bw2, H * 0.30f / bh2);
        mapW = bw2 * mapScale; mapH = bh2 * mapScale;
        mapX = W - m - pad - mapW; mapY = m + pad;
    }

    if (state == GameState::PLAYING || state == GameState::PAUSED)
    {
        // sun burn flash, and a pulsing red frame when health is low
        if (burn > 0.02f) hud.Rect(0, 0, W, H, glm::vec4(1.f, 0.15f, 0.0f, 0.28f * burn));
        if (health < maxHealth * 0.3f)
        {
            float a2 = 0.25f + 0.2f * (float)std::sin(timer * 9.0), e = 3.f * s;
            glm::vec4 red(0.9f, 0.05f, 0.05f, a2);
            hud.Rect(0, 0, W, e, red); hud.Rect(0, H - e, W, e, red); hud.Rect(0, 0, e, H, red); hud.Rect(W - e, 0, e, H, red);
        }

        // ------- top-left panel: health, stamina, sun/shade -------
        const float barW = 60.f * s, barH = 6.f * s, stH = 3.f * s;
        float top = H - m;                               // top edge of the content
        float yLabel = top - th;                         // "HEALTH"
        float yBar = yLabel - 2.f * s - barH;
        float yStam = yBar - 3.f * s - stH;
        float yInd = yStam - 4.f * s - th;
        float yWarn = yInd - 3.f * s - th;
        float contentW = barW + 3.f * s + hud.TextWidth("100", s);
        contentW = std::max(contentW, 9.f * s + hud.TextWidth("SHADE", s));
        if (outsideShadowBox) contentW = std::max(contentW, hud.TextWidth("OUTSIDE SHADOW BOX", s));
        float bottom = outsideShadowBox ? yWarn : yInd;
        hud.Rect(m - pad, bottom - pad, contentW + 2.f * pad, (top - bottom) + 2.f * pad, panel);

        hud.Text("HEALTH", m, yLabel, s, white);
        hud.Rect(m, yBar, barW, barH, glm::vec4(0, 0, 0, 0.55f));
        float k = glm::clamp(health / std::max(1.f, maxHealth), 0.f, 1.f);
        hud.Rect(m, yBar, barW * k, barH, glm::mix(glm::vec4(0.9f, 0.1f, 0.05f, 1.f), glm::vec4(0.2f, 0.85f, 0.3f, 1.f), k));
        hud.Text(Num(health, 0), m + barW + 3.f * s, yBar + (barH - th) * 0.5f, s, white);

        float sk = glm::clamp(stamina / std::max(1.f, maxStamina), 0.f, 1.f);
        glm::vec4 stCol = exhausted ? glm::vec4(0.85f, 0.3f, 0.2f, 1.f) : glm::vec4(0.95f, 0.85f, 0.25f, 1.f);
        hud.Rect(m, yStam, barW, stH, glm::vec4(0, 0, 0, 0.55f));
        hud.Rect(m, yStam, barW * sk, stH, stCol);
        hud.Text(exhausted ? "TIRED" : "RUN", m + barW + 3.f * s, yStam + (stH - th) * 0.5f, s, stCol);

        bool sun = litFraction > 0.5f;
        glm::vec4 ic = sun ? glm::vec4(1.f, 0.65f, 0.1f, 1.f) : glm::vec4(0.4f, 0.65f, 1.f, 1.f);
        hud.Rect(m, yInd, th, th, ic);
        hud.Text(sun ? "SUN" : "SHADE", m + 9.f * s, yInd, s, ic);
        if (outsideShadowBox) hud.Text("OUTSIDE SHADOW BOX", m, yWarn, s, glm::vec4(1.f, 0.35f, 0.35f, 1.f));

        // ------- top-right panel: level, time, distance to the goal (right aligned) -------
        std::string lv = "LEVEL " + std::to_string(levelNumber), t = "TIME " + Num(timer, 1);
        std::string g = "GOAL " + Num(glm::distance(glm::vec2(feetPos.x, feetPos.z), goal), 0);
        float rw = std::max(hud.TextWidth(lv, s), std::max(hud.TextWidth(t, s), hud.TextWidth(g, s)));
        float rTop = H - m, rBottom = rTop - th - 2.f * lineH;
        hud.Rect(W - m - rw - pad, rBottom - pad, rw + 2.f * pad, (rTop - rBottom) + 2.f * pad, panel);
        hud.Text(lv, W - m - hud.TextWidth(lv, s), rTop - th, s, white);
        hud.Text(t, W - m - hud.TextWidth(t, s), rTop - th - lineH, s, white);
        hud.Text(g, W - m - hud.TextWidth(g, s), rTop - th - 2.f * lineH, s, glm::vec4(0.55f, 0.8f, 1.f, 1.f));

        // crosshair
        hud.Rect(W * 0.5f - 1, H * 0.5f - 6, 2, 12, glm::vec4(1, 1, 1, 0.6f));
        hud.Rect(W * 0.5f - 6, H * 0.5f - 1, 12, 2, glm::vec4(1, 1, 1, 0.6f));

        // ------- tips: wrapped, centred in the space LEFT of the map, one every six seconds at the start -------
        if (!hints.empty() && state == GameState::PLAYING)
        {
            size_t idx = (size_t)(timer / 6.f);
            if (idx < hints.size())
            {
                float hs = std::max(2.f, std::floor(s * 1.5f));           // a whole number of pixels per font pixel
                float areaL = m, areaR = showMap ? (mapX - pad - m) : (W - m);
                float areaW = std::max(100.f, areaR - areaL);
                std::vector<std::string> ls = WrapText(hud, hints[idx], hs, areaW - 2.f * pad);
                float blockW = 0.f;
                for (const std::string& l : ls) blockW = std::max(blockW, hud.TextWidth(l, hs));
                float rowH = 10.f * hs, cx = areaL + areaW * 0.5f;
                float blockH = (float)ls.size() * rowH - 3.f * hs;
                hud.Rect(cx - blockW * 0.5f - pad, m - pad, blockW + 2.f * pad, blockH + 2.f * pad, panel);
                for (size_t i = 0; i < ls.size(); ++i)
                    centredAt(ls[i], cx, m + (float)(ls.size() - 1 - i) * rowH, hs, glm::vec4(1.f, 0.95f, 0.6f, 1.f));
            }
        }

        // ------- top-down map (Tab), bottom-right, everything clipped to its border -------
        if (showMap)
        {
            hud.Rect(mapX - pad, mapY - pad, mapW + 2.f * pad, mapH + 2.f * pad, glm::vec4(0.03f, 0.03f, 0.06f, 0.8f));
            hud.Rect(mapX, mapY, mapW, mapH, glm::vec4(0.8f, 0.7f, 0.5f, 1.f));
            const float yTop = mapY + mapH;                              // +Z runs DOWN the map, like the 3D view facing +X
            for (const ObjectDef& o : objs)
            {
                if (!o.solid) continue;
                float x0 = std::max(o.pos.x - o.scale.x * 0.5f, bounds.x), x1 = std::min(o.pos.x + o.scale.x * 0.5f, bounds.z);
                float z0 = std::max(o.pos.z - o.scale.z * 0.5f, bounds.y), z1 = std::min(o.pos.z + o.scale.z * 0.5f, bounds.w);
                if (x1 <= x0 || z1 <= z0) continue;
                float rw2 = std::max((x1 - x0) * mapScale, s), rh2 = std::max((z1 - z0) * mapScale, s);
                float rx = mapX + (x0 - bounds.x) * mapScale;
                float ry = yTop - (z0 - bounds.y) * mapScale - rh2;
                rx = std::min(rx, mapX + mapW - rw2); ry = std::max(ry, mapY);       // keep thin walls inside the border
                hud.Rect(rx, ry, rw2, rh2, o.movable ? glm::vec4(1.f, 0.5f, 0.05f, 1.f) : glm::vec4(0.22f, 0.22f, 0.28f, 1.f));
            }
            float gx = mapX + (goal.x - bounds.x) * mapScale, gy = yTop - (goal.y - bounds.y) * mapScale;
            hud.Rect(gx - 2.f * s, gy - 2.f * s, 4.f * s, 4.f * s, glm::vec4(0.2f, 0.6f, 1.f, 1.f));
            float px = mapX + (feetPos.x - bounds.x) * mapScale, py = yTop - (feetPos.z - bounds.y) * mapScale;
            glm::vec2 fd(camFront.x, camFront.z);
            if (glm::length(fd) > 1e-4f) fd = glm::normalize(fd);
            hud.Rect(px + fd.x * 3.5f * s - 1.f * s, py - fd.y * 3.5f * s - 1.f * s, 2.f * s, 2.f * s, glm::vec4(1.f, 1.f, 1.f, 1.f));   // where you look
            hud.Rect(px - 1.5f * s, py - 1.5f * s, 3.f * s, 3.f * s, glm::vec4(1.f, 0.9f, 0.1f, 1.f));                                     // you
        }
    }

    // ---------------- full-screen states: one centred column built from a cursor ----------------
    if (state == GameState::TITLE)
    {
        hud.Rect(0, 0, W, H, glm::vec4(0.02f, 0.02f, 0.05f, 0.85f));
        float y = H * 0.78f;
        centred("UMBRA", y, s * 3.f, glm::vec4(1.f, 0.8f, 0.3f, 1.f));                       y -= 7.f * s * 3.f + 5.f * s;
        centred("SHADOW IS SOLID GROUND", y, s, glm::vec4(0.7f, 0.8f, 1.f, 1.f));          y -= 4.f * lineH;
        centred("LEVEL " + std::to_string(levelNumber) + ": " + levelName, y, s * 2.f, white); y -= 3.f * lineH;
        const char* lines[] = { "THE SUN BURNS YOU. SHADE HEALS YOU.", "REACH THE BLUE LIGHT.",
                                "WASD MOVE   MOUSE LOOK   SHIFT RUN", "PUSH THE ORANGE BLOCKS TO MAKE NEW SHADE",
                                "TAB MAP   R RESTART   P PAUSE   F1 DEBUG" };
        for (int i = 0; i < 5; ++i) { centred(lines[i], y, s, white); y -= lineH; }
        y -= 2.f * lineH;
        centred("PRESS ENTER TO START", y, s * 2.f, glm::vec4(1.f, 0.9f, 0.4f, 1.f));      y -= 3.f * lineH;
        auto it = bestTimes.find(levelName);
        if (it != bestTimes.end()) centred("BEST TIME " + Num(it->second, 1), y, s, glm::vec4(0.6f, 1.f, 0.6f, 1.f));
        if (warnings > 0) centred("SCENE PROBLEMS: " + std::to_string(warnings) + "   SEE CONSOLE", m, s, glm::vec4(1.f, 0.4f, 0.4f, 1.f));
    }
    else if (state == GameState::PAUSED)
    {
        hud.Rect(0, 0, W, H, glm::vec4(0, 0, 0, 0.5f));
        float y = H * 0.58f;
        centred("PAUSED", y, s * 3.f, white);                                                 y -= 5.f * lineH;
        centred("P OR ESC TO RESUME   R TO RESTART", y, s, white);
    }
    else if (state == GameState::WON)
    {
        hud.Rect(0, 0, W, H, glm::vec4(0.0f, 0.1f, 0.25f, 0.62f));
        float y = H * 0.64f;
        centred("YOU WIN", y, s * 3.f, glm::vec4(0.5f, 0.85f, 1.f, 1.f));                    y -= 5.f * lineH;
        for (int i = 0; i < 3; ++i)   // stars: filled = earned
            hud.Rect(W * 0.5f + (float)(i - 1) * 12.f * s - 4.f * s, y, 8.f * s, 8.f * s,
                     i < stars ? glm::vec4(1.f, 0.85f, 0.2f, 1.f) : glm::vec4(0.3f, 0.3f, 0.35f, 1.f));
        y -= 2.f * lineH;
        centred("TIME " + Num(timer, 1) + "   PAR " + Num(parTime, 0) + "   HEALTH " + Num(health, 0), y, s, white); y -= lineH;
        auto it = bestTimes.find(levelName);
        if (it != bestTimes.end()) { centred("BEST " + Num(it->second, 1), y, s, glm::vec4(0.6f, 1.f, 0.6f, 1.f)); y -= lineH; }
        y -= lineH;
        centred(nextLevel.empty() ? "ENTER: START AGAIN AT LEVEL 1" : "ENTER: NEXT LEVEL", y, s, glm::vec4(1.f, 0.9f, 0.4f, 1.f)); y -= lineH;
        centred("R: REPLAY THIS LEVEL", y, s, white);
    }
    else if (state == GameState::LOST)
    {
        hud.Rect(0, 0, W, H, glm::vec4(0.35f, 0.02f, 0.0f, 0.65f));
        float y = H * 0.6f;
        centred("YOU BURNED", y, s * 3.f, glm::vec4(1.f, 0.45f, 0.2f, 1.f));                 y -= 5.f * lineH;
        centred("STAY IN THE SHADE", y, s * 2.f, white);                                      y -= 3.f * lineH;
        centred("ENTER OR R TO TRY AGAIN", y, s, white);
    }

    // ---------------- F1 debug panel: bottom-left ----------------
    if (!debug.empty())
    {
        float dw = 0.f;
        for (const std::string& d : debug) dw = std::max(dw, hud.TextWidth(d, s));
        float dh = (float)debug.size() * lineH - 3.f * s;
        hud.Rect(m - pad, m + 14.f * s - pad, dw + 2.f * pad, dh + 2.f * pad, panel);   // lifted above the tips line
        float dy = m + 14.f * s;
        for (size_t i = debug.size(); i-- > 0;) { hud.Text(debug[i], m, dy, s, glm::vec4(0.9f, 1.f, 0.6f, 1.f)); dy += lineH; }
    }
}
