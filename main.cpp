// =============================================================================
//  UMBRA - "Shadow as solid ground".   Group 3, Computer Graphics Programming.
//  main.cpp  (owner: R1 Technical Director)
//
//  ONE FRAME, in order (report section S1; docs/S1_frame_draft.txt has the long version):
//    1  poll input, run the game logic           (CPU)
//    2  sun direction + shadow box transform     (CPU)
//    3  sun shadow pass       directional_shadow_map.vert/.frag     (only when dirty)
//    4  point-light shadow passes  omni_shadow_map.vert/.geom/.frag (only when dirty)
//    5  main pass             shader.vert / shader.frag
//    6  skybox                skybox.vert / skybox.frag
//    7  sun probe             sunprobe.vert / sunprobe.frag  (1x1 framebuffer -> health)
//    8  HUD                   hud.vert / hud.frag            (orthographic, one draw call)
// =============================================================================
#include "CommonValues.h"
#include "Window.h"
#include "Camera.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Skybox.h"
#include "SceneLoader.h"
#include "SunProbe.h"
#include "HUD.h"
#include "Game.h"

#include <map>
#include <memory>
#include <ctime>
#ifdef _WIN32
#include <direct.h>
#define UMBRA_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define UMBRA_MKDIR(p) mkdir(p, 0755)
#endif

static Mesh *gBox = nullptr, *gPlane = nullptr, *gCylinder = nullptr, *gPyramid = nullptr;

// Everything that belongs to one loaded level.
struct Level
{
    SceneData sd;
    std::vector<ObjectDef> original;          // as loaded (used by restart)
    std::vector<ObjectDef> objects;           // live copy: movable blocks change position
    std::map<std::string, Texture*> textures;
    std::map<std::string, Material*> materials;
    DirectionalLight sun;
    std::vector<PointLight*> points;
    std::vector<SpotLight*> spots;
    Skybox sky;
    Level() {}
    ~Level()
    {
        for (auto& t : textures) delete t.second;
        for (auto& m : materials) delete m.second;
        for (PointLight* p : points) delete p;
        for (SpotLight* s : spots) delete s;
    }
private:
    Level(const Level&); Level& operator=(const Level&);
};

static void LogProblem(SceneData& sd, const std::string& m)
{
    std::fprintf(stderr, "ERROR  %s\n", m.c_str());
    sd.messages.push_back(m);
    sd.errorCount++;
}

// Builds every GPU object a scene file describes.  Returns nullptr if the file is unusable.
static Level* BuildLevel(const std::string& path)
{
    std::unique_ptr<Level> L(new Level());
    SceneData& sd = L->sd;
    if (!LoadScene(path, sd)) return nullptr;

    // placeholders so a bad reference is visible (magenta) instead of silently black
    Texture* missing = new Texture(); missing->GenerateMissing();
    L->textures["__missing"] = missing;
    Material* def = new Material(); def->name = "__default"; def->texture = missing;
    L->materials["__default"] = def;

    for (const TextureDef& td : sd.textures)
    {
        Texture* t = new Texture();
        if (td.isFile && td.missing) t->GenerateMissing();       // already reported (with its line number) by the loader
        else if (td.isFile)
        {
            std::string err;
            if (!t->LoadTGA(td.path, err))
            {
                LogProblem(sd, "scene file '" + path + "' line " + std::to_string(td.line) + ": texture '" + td.name +
                               "' file '" + td.path + "': " + err + " -> using the magenta placeholder");
                t->GenerateMissing();
            }
        }
        else t->GenerateProcedural(td.type, td.c1, td.c2, td.size);
        L->textures[td.name] = t;
    }

    for (const MaterialDef& md : sd.materials)
    {
        Material* m = new Material();
        m->name = md.name;
        auto it = L->textures.find(md.texture);
        m->texture = (it != L->textures.end()) ? it->second : missing;
        m->specularIntensity = md.specular; m->shininess = md.shininess; m->emissive = md.emissive;
        L->materials[md.name] = m;
    }

    L->objects = sd.objects;
    for (ObjectDef& o : L->objects)
    {
        o.mesh = (o.shape == "box") ? gBox : (o.shape == "plane") ? gPlane : (o.shape == "cylinder") ? gCylinder : gPyramid;
        auto it = L->materials.find(o.material);
        o.mat = (it != L->materials.end()) ? it->second : def;
    }
    L->original = L->objects;

    const SunDef& s = sd.sun;
    if (!L->sun.Init(s.dir, s.colour, s.ambient, s.diffuse, s.halfSize, s.nearPlane, s.farPlane, s.resolution))
    { LogProblem(sd, "could not create the sun shadow map"); return nullptr; }

    double bytes = (double)s.resolution * s.resolution * 4.0;
    for (const PointDef& pd : sd.points)
    {
        PointLight* p = new PointLight();
        if (!p->Init(pd.pos, pd.colour, pd.ambient, pd.diffuse, pd.constant, pd.linear, pd.exponent, pd.farPlane, pd.resolution))
        { LogProblem(sd, "could not create a point-light shadow map (line " + std::to_string(pd.line) + ")"); delete p; continue; }
        L->points.push_back(p);
        bytes += (double)pd.resolution * pd.resolution * 6.0 * 4.0;
    }
    for (const SpotDef& sp : sd.spots)
    {
        SpotLight* l = new SpotLight();
        l->position = sp.pos; l->direction = glm::normalize(sp.dir); l->colour = sp.colour;
        l->ambientIntensity = sp.ambient; l->diffuseIntensity = sp.diffuse;
        l->constant = sp.constant; l->linear = sp.linear; l->exponent = sp.exponent;
        l->SetEdge(sp.edgeDeg); l->attachToCamera = sp.attachToCamera;
        L->spots.push_back(l);
    }
    if (!L->sky.Init(sd.skyFaces, 256)) { LogProblem(sd, "could not build the skybox"); return nullptr; }

    std::printf("Level '%s': %d objects, %d materials, %d point lights, %d spot lights, shadow maps %.1f MiB, %d scene problem(s)\n",
                sd.name.c_str(), (int)L->objects.size(), (int)sd.materials.size(), (int)L->points.size(),
                (int)L->spots.size(), bytes / (1024.0 * 1024.0), sd.errorCount);
    return L.release();
}

// Draws every object.  Shadow passes only need the model matrix; the main pass needs everything.
static void RenderObjects(Shader& s, const std::vector<ObjectDef>& objs, bool shadowPass)
{
    for (const ObjectDef& o : objs)
    {
        if (shadowPass && !o.castShadow) continue;
        glm::mat4 model = glm::translate(glm::mat4(1.f), o.pos);
        model = glm::scale(model, o.scale);
        s.SetMat4("model", model);
        if (!shadowPass)
        {
            s.SetMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
            s.SetVec2("uvScale", o.tile);
            o.mat->Use(s);
        }
        o.mesh->Render();
    }
}

static std::string F(float v, int d)
{
    char b[32]; std::snprintf(b, sizeof(b), "%.*f", d, v); return b;
}

// F12 / K: saves what is on screen as a 24-bit BMP in ./screenshots (for the proof screenshots).
static void SaveScreenshot(int w, int h)
{
    UMBRA_MKDIR("screenshots");
    size_t rowBytes = ((size_t)w * 3 + 3) / 4 * 4;
    std::vector<unsigned char> px(rowBytes * h);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, px.data());   // bottom row first = BMP order
    char name[96];
    std::snprintf(name, sizeof(name), "screenshots/umbra_%lld.bmp", (long long)std::time(nullptr));
    std::ofstream f(name, std::ios::binary);
    if (!f.good()) { std::fprintf(stderr, "Could not write %s\n", name); return; }
    auto w32 = [&](unsigned int v) { for (int i = 0; i < 4; ++i) f.put((char)((v >> (8 * i)) & 0xFF)); };
    auto w16 = [&](unsigned int v) { for (int i = 0; i < 2; ++i) f.put((char)((v >> (8 * i)) & 0xFF)); };
    unsigned int dataSize = (unsigned int)px.size();
    f.put('B'); f.put('M'); w32(54 + dataSize); w32(0); w32(54);
    w32(40); w32((unsigned int)w); w32((unsigned int)h); w16(1); w16(24); w32(0); w32(dataSize); w32(2835); w32(2835); w32(0); w32(0);
    f.write((const char*)px.data(), (std::streamsize)px.size());
    std::printf("Saved %s (%dx%d)\n", name, w, h);
}

static void PrintControls()
{
    std::printf(
        "\n=== UMBRA controls ===\n"
        " WASD move, mouse look, SHIFT run, push the ORANGE blocks to make new shade, TAB map\n"
        " ENTER start/continue   R restart   P or ESC pause\n"
        " --- proof / demo keys ---\n"
        " F1  debug panel          F2  free camera (no damage)   F3  PCF off/3x3/5x5\n"
        " 1   bias TOO SMALL (acne)   2  bias about right   3  bias TOO LARGE (peter-panning)\n"
        " 0   switch the sun's shadows OFF (the game turns lethal everywhere: shadows are not decoration)\n"
        " Z / X  halve / double the shadow box half-size (P2, P3)\n"
        " N   shadow box follows the player (E1)    M  snap box to whole texels on/off (E1)\n"
        " G   re-render all shadow maps every frame vs only when something changed\n"
        " C   (free camera) look along the next of +X -X +Y -Y +Z -Z   (cube map proof screenshots)\n"
        " F12 or K  save a screenshot to ./screenshots      T flashlight    V vsync on/off\n"
        "======================\n\n");
}

int main(int argc, char** argv)
{
    std::string startLevel = "Scenes/level1.txt";
    if (argc > 1) startLevel = argv[1];

    Window window(1280, 720);
    if (!window.Init()) { std::fprintf(stderr, "Press Enter to close.\n"); std::getchar(); return 1; }
    window.SetCursorCaptured(false);

    Shader mainShader, dirShader, omniShader;
    bool ok = mainShader.CreateFromFiles("Shaders/shader.vert", "", "Shaders/shader.frag")
           && dirShader.CreateFromFiles("Shaders/directional_shadow_map.vert", "", "Shaders/directional_shadow_map.frag")
           && omniShader.CreateFromFiles("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
    HUD hud; SunProbe probe;
    ok = ok && hud.Init() && probe.Init();
    if (!ok)
    {
        std::fprintf(stderr, "A shader or GPU resource failed to build (see messages above). Is the Shaders folder next to the program?\nPress Enter to close.\n");
        std::getchar();
        return 1;
    }

    gBox = Mesh::MakeBox(); gPlane = Mesh::MakePlane(); gCylinder = Mesh::MakeCylinder(24); gPyramid = Mesh::MakePyramid();
    std::printf("Meshes (vertices/indices): box %d/%d (24 = 4 per face, one normal per face), plane %d/%d, cylinder %d/%d, pyramid %d/%d\n",
                gBox->VertexCount(), gBox->IndexCount(), gPlane->VertexCount(), gPlane->IndexCount(),
                gCylinder->VertexCount(), gCylinder->IndexCount(), gPyramid->VertexCount(), gPyramid->IndexCount());

    int levelNumber = 1;
    std::unique_ptr<Level> level(BuildLevel(startLevel));
    if (!level)
    {
        std::fprintf(stderr, "The level '%s' could not be loaded (see messages above).\nPress Enter to close.\n", startLevel.c_str());
        std::getchar();
        return 1;
    }

    // A tiny cleared cube map for the unused point-light slots, so no samplerCube is ever left pointing at an empty unit.
    OmniShadowMap dummyCube;
    if (dummyCube.Init(4)) { dummyCube.Write(); glViewport(0, 0, 4, 4); glClear(GL_DEPTH_BUFFER_BIT); glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    Camera camera(glm::vec3(0.f, 1.7f, 0.f), 0.f, -5.f, 0.12f);
    Game game;
    game.LoadLevel(level->sd, level->original, level->objects, levelNumber);

    // ---- runtime settings that the proof keys change ----
    float biasScale = 0.f, biasMin = 0.f; int biasPreset = 2;
    int pcfRadius = 0, lookIndex = 0;
    bool follow = false, snap = true, alwaysRerender = false, debugHud = false, vsync = true, shadowsOn = true;
    bool dirDirty = true, omniDirty = true, captured = false, takeShot = false;
    double sunTime = 0.0;
    glm::vec3 baseSunDir(0.f, -0.6f, 0.8f);

    auto applyLevelSettings = [&]() {
        const SunDef& s = level->sd.sun;
        biasScale = s.biasScale; biasMin = s.biasMin; biasPreset = 2;
        follow = s.follow; snap = s.snap; shadowsOn = true;
        baseSunDir = glm::normalize(s.dir);
        sunTime = 0.0; dirDirty = omniDirty = true;
        camera.setPosition(game.feetPos + glm::vec3(0.f, level->sd.eyeHeight, 0.f));
    };
    applyLevelSettings();
    PrintControls();

    // GPU timer queries are not available on every driver (macOS has none): fall back to wall/CPU time only.
    const bool haveGpuTimer = (GLEW_ARB_timer_query != 0);
    GLuint timerQuery[2] = { 0, 0 };
    if (haveGpuTimer) glGenQueries(2, timerQuery);
    else std::printf("GPU timer queries unavailable on this driver: GPU ms will show 0 (wall and CPU times still work).\n");
    int queryIndex = 0; unsigned long long frameCounter = 0;
    double lastTime = glfwGetTime();
    double accWall = 0, accGpu = 0, accCpu = 0; int accCount = 0; double lastAvgWall = 0, lastAvgGpu = 0, lastAvgCpu = 0;

    while (!window.ShouldClose())
    {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        if (dt > 0.05f) dt = 0.05f;                 // a hitch must not teleport the player through a wall
        double cpuStart = glfwGetTime();

        glfwPollEvents();
        int bw = window.GetBufferWidth(), bh = window.GetBufferHeight();
        if (bw == 0 || bh == 0) { glfwWaitEvents(); continue; }   // minimised
        Level& L = *level;
        DirectionalLight& sun = L.sun;

        // ---------- debug / proof keys ----------
        if (window.Pressed(GLFW_KEY_F1)) debugHud = !debugHud;
        if (window.Pressed(GLFW_KEY_F2))
        {
            game.freeCam = !game.freeCam;
            if (!game.freeCam) camera.setPosition(game.feetPos + glm::vec3(0.f, L.sd.eyeHeight, 0.f));
        }
        if (window.Pressed(GLFW_KEY_F3)) { pcfRadius = (pcfRadius + 1) % 3; }
        if (window.Pressed(GLFW_KEY_1)) { biasScale = 0.f;          biasMin = 0.00002f; biasPreset = 1; std::printf("Bias preset 1: TOO SMALL (acne)\n"); }
        if (window.Pressed(GLFW_KEY_2)) { biasScale = L.sd.sun.biasScale; biasMin = L.sd.sun.biasMin; biasPreset = 2; std::printf("Bias preset 2: about right (from scene file)\n"); }
        if (window.Pressed(GLFW_KEY_3)) { biasScale = 0.f;          biasMin = 0.02f;    biasPreset = 3; std::printf("Bias preset 3: TOO LARGE (peter-panning)\n"); }
        if (window.Pressed(GLFW_KEY_0)) { shadowsOn = !shadowsOn; std::printf("Sun shadows: %s\n", shadowsOn ? "ON" : "OFF (nothing blocks the sun)"); }
        if (window.Pressed(GLFW_KEY_Z)) { sun.halfSize = std::max(1.f, sun.halfSize * 0.5f); dirDirty = true;
            std::printf("Shadow box half-size %.2f -> %.4f world units per texel\n", sun.halfSize, sun.TexelSize()); }
        if (window.Pressed(GLFW_KEY_X)) { sun.halfSize = std::min(400.f, sun.halfSize * 2.0f); dirDirty = true;
            std::printf("Shadow box half-size %.2f -> %.4f world units per texel\n", sun.halfSize, sun.TexelSize()); }
        if (window.Pressed(GLFW_KEY_N)) { follow = !follow; dirDirty = true; std::printf("Shadow box follows player: %s\n", follow ? "ON" : "OFF"); }
        if (window.Pressed(GLFW_KEY_M)) { snap = !snap; std::printf("Texel snapping: %s\n", snap ? "ON" : "OFF"); }
        if (window.Pressed(GLFW_KEY_G)) { alwaysRerender = !alwaysRerender; std::printf("Re-render all shadow maps every frame: %s\n", alwaysRerender ? "ON" : "OFF"); }
        if (window.Pressed(GLFW_KEY_V)) { vsync = !vsync; window.SetVsync(vsync); std::printf("VSync: %s\n", vsync ? "ON" : "OFF"); }
        if (window.Pressed(GLFW_KEY_T)) { for (SpotLight* s : L.spots) if (s->attachToCamera) s->enabled = !s->enabled; }
        if (window.Pressed(GLFW_KEY_F12) || window.Pressed(GLFW_KEY_K)) takeShot = true;
        if (window.Pressed(GLFW_KEY_C))
        {
            if (!game.freeCam) std::printf("Press F2 for the free camera first, then C to look along each axis.\n");
            else
            {
                static const float yaw[6]   = {   0.f, 180.f,  0.f,   0.f,  90.f, -90.f };
                static const float pitch[6] = {   0.f,   0.f, 89.9f, -89.9f, 0.f,   0.f };
                static const char* nm[6] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
                camera.setAngles(yaw[lookIndex], pitch[lookIndex]);
                std::printf("Camera looking along %s\n", nm[lookIndex]);
                lookIndex = (lookIndex + 1) % 6;
            }
        }

        // ---------- mouse / cursor ----------
        bool wantCapture = (game.state == GameState::PLAYING);
        if (wantCapture != captured) { window.SetCursorCaptured(wantCapture); captured = wantCapture; }
        float dx = window.GetXChange(), dy = window.GetYChange();
        if (captured) camera.mouseControl(dx, dy);
        if (captured && game.freeCam) camera.flyControl(window.GetKeys(), dt, 8.f);

        // ---------- game logic ----------
        game.Update(window, camera, L.objects, dt, probe.LitFraction(), probe.OutsideBox());
        if (game.wantQuit) { window.SetShouldClose(); }
        if (game.wantRestart)
        {
            game.Restart(L.original, L.objects);
            applyLevelSettings();
        }
        if (game.wantNextLevel || game.wantFirstLevel)
        {
            std::string target = game.wantNextLevel ? game.nextLevel : startLevel;
            int number = game.wantNextLevel ? levelNumber + 1 : 1;
            std::unique_ptr<Level> next(BuildLevel(target));
            if (next)
            {
                level.reset(next.release());
                levelNumber = number;
                game.LoadLevel(level->sd, level->original, level->objects, levelNumber);
                applyLevelSettings();
                continue;   // restart the frame with the new level (references above are stale)
            }
            std::fprintf(stderr, "Could not load level '%s'; replaying this one.\n", target.c_str());
            game.Restart(L.original, L.objects);
            applyLevelSettings();
        }
        if (game.movedObjects) { dirDirty = true; omniDirty = true; }

        // ---------- the sun ----------
        if (game.state == GameState::PLAYING) sunTime += dt;
        if (L.sd.sun.sweepAmplitudeDeg > 0.f)
        {
            float ang = L.sd.sun.sweepAmplitudeDeg * (float)std::sin(2.0 * 3.14159265358979 * sunTime / L.sd.sun.sweepPeriodSec);
            glm::vec4 d = glm::rotate(glm::mat4(1.f), ang * TO_RADIANS, glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(baseSunDir, 0.f);
            sun.SetDirection(glm::vec3(d));
            if (game.state == GameState::PLAYING) dirDirty = true;
        }
        glm::vec3 boxCentre = L.sd.sun.centre;
        if (follow)
        {
            glm::vec3 anchor = game.freeCam ? camera.getPosition() : game.feetPos;
            boxCentre = anchor + camera.getFlatForward() * L.sd.sun.followAhead;     // E1: centred AHEAD of the player
            dirDirty = true;
        }
        sun.CalcLightTransform(boxCentre, follow && snap);

        // ---------- GPU timer ----------
        if (haveGpuTimer) glBeginQuery(GL_TIME_ELAPSED, timerQuery[queryIndex]);

        // ---------- shadow passes: only redraw what changed ----------
        bool doDir = dirDirty || alwaysRerender;
        bool doOmni = omniDirty || alwaysRerender;
        if (doDir)
        {
            glViewport(0, 0, sun.resolution, sun.resolution);
            sun.GetShadowMap()->Write();
            glClear(GL_DEPTH_BUFFER_BIT);
            dirShader.UseShader();
            dirShader.SetMat4("directionalLightTransform", sun.GetTransform());
            RenderObjects(dirShader, L.objects, true);
            dirDirty = false;
        }
        if (doOmni)
        {
            omniShader.UseShader();
            for (PointLight* p : L.points)
            {
                glViewport(0, 0, p->resolution, p->resolution);
                p->GetShadowMap()->Write();
                glClear(GL_DEPTH_BUFFER_BIT);
                omniShader.SetVec3("lightPos", p->position);
                omniShader.SetFloat("farPlane", p->farPlane);
                omniShader.SetMat4Array("lightMatrices", p->CalcLightTransforms());
                RenderObjects(omniShader, L.objects, true);   // ONE draw pass per light; the geometry shader fans out to 6 faces
            }
            omniDirty = false;
        }

        // ---------- main pass ----------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, bw, bh);
        glClearColor(0.55f, 0.65f, 0.85f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(65.0f), (GLfloat)bw / (GLfloat)bh, 0.1f, 300.f);   // B3
        glm::mat4 view = camera.calculateViewMatrix();

        for (SpotLight* s : L.spots)
            if (s->attachToCamera) { s->position = camera.getPosition() - glm::vec3(0.f, 0.15f, 0.f); s->direction = camera.getFront(); }

        mainShader.UseShader();
        mainShader.SetMat4("projection", projection);
        mainShader.SetMat4("view", view);
        mainShader.SetVec3("eyePosition", camera.getPosition());
        mainShader.SetMat4("directionalLightTransform", sun.GetTransform());
        mainShader.SetDirectionalLight(sun);
        mainShader.SetPointLights(L.points, UNIT_OMNI_FIRST);
        mainShader.SetSpotLights(L.spots);
        mainShader.SetInt("theTexture", UNIT_DIFFUSE);
        mainShader.SetInt("directionalShadowMap", UNIT_DIR_SHADOW);
        mainShader.SetFloat("dirBiasScale", biasScale);
        mainShader.SetFloat("dirBiasMin", biasMin);
        mainShader.SetFloat("omniBias", L.sd.omniBias);
        mainShader.SetInt("pcfRadius", pcfRadius);
        mainShader.SetInt("shadowsEnabled", shadowsOn ? 1 : 0);
        mainShader.SetVec3("fogColour", L.sd.fogColour);
        mainShader.SetFloat("fogDensity", L.sd.fogDensity);
        sun.GetShadowMap()->Read(GL_TEXTURE0 + UNIT_DIR_SHADOW);
        for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
        {
            if (i < (int)L.points.size()) L.points[i]->GetShadowMap()->Read(GL_TEXTURE0 + UNIT_OMNI_FIRST + i);
            else dummyCube.Read(GL_TEXTURE0 + UNIT_OMNI_FIRST + i);
        }
        RenderObjects(mainShader, L.objects, false);

        L.sky.Draw(view, projection, sun.ToSun());

        // ---------- sun probe: this is what hurts the player ----------
        probe.Query(sun, game.feetPos, L.sd.playerRadius, biasScale, biasMin, shadowsOn, bw, bh);

        // ---------- HUD ----------
        std::vector<std::string> dbg;
        if (debugHud)
        {
            dbg.push_back("FRAME " + F((float)lastAvgWall, 2) + " MS  GPU " + F((float)lastAvgGpu, 2) + " MS  CPU " + F((float)lastAvgCpu, 2) + " MS");
            dbg.push_back(std::string("BIAS PRESET ") + std::to_string(biasPreset) + "  SCALE " + F(biasScale, 5) + "  MIN " + F(biasMin, 5) + (shadowsOn ? "" : "  SHADOWS OFF"));
            dbg.push_back("BOX HALF " + F(sun.halfSize, 1) + "  TEXEL " + F(sun.TexelSize(), 4) + "  RES " + std::to_string(sun.resolution));
            dbg.push_back(std::string("FOLLOW ") + (follow ? "ON" : "OFF") + "  SNAP " + (snap ? "ON" : "OFF") + "  PCF " + std::to_string(pcfRadius));
            dbg.push_back(std::string("SHADOW REDRAW ") + (alwaysRerender ? "ALWAYS" : "ONLY WHEN DIRTY") + "  SUN LIT " + F(probe.LitFraction(), 2));
        }
        hud.Begin(bw, bh);
        game.DrawHUD(hud, bw, bh, dbg, L.sd.errorCount, L.objects, camera.getFront());
        hud.End();

        if (haveGpuTimer) glEndQuery(GL_TIME_ELAPSED);
        if (takeShot) { SaveScreenshot(bw, bh); takeShot = false; }
        double cpuMs = (glfwGetTime() - cpuStart) * 1000.0;
        window.SwapBuffers();

        // ---------- timing harness (R5): averages over 300 frames, in milliseconds ----------
        double gpuMs = 0.0;
        if (haveGpuTimer && frameCounter > 0)
        {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(timerQuery[1 - queryIndex], GL_QUERY_RESULT, &ns);
            gpuMs = (double)ns / 1.0e6;
        }
        queryIndex = 1 - queryIndex;
        ++frameCounter;
        accWall += dt * 1000.0; accGpu += gpuMs; accCpu += cpuMs; ++accCount;
        if (accCount == 300)
        {
            lastAvgWall = accWall / 300.0; lastAvgGpu = accGpu / 300.0; lastAvgCpu = accCpu / 300.0;
            std::printf("[300-frame average] wall %.2f ms (%.0f fps) | GPU %.2f ms | CPU submit %.2f ms | vsync %s | shadows %s | level %d\n",
                        lastAvgWall, 1000.0 / lastAvgWall, lastAvgGpu, lastAvgCpu, vsync ? "on" : "off", alwaysRerender ? "every frame" : "when dirty", levelNumber);
            window.SetTitle("UMBRA | L" + std::to_string(levelNumber) + " " + L.sd.name + " | " + F((float)lastAvgWall, 2) + " ms | GL " +
                            std::to_string(window.glMajor) + "." + std::to_string(window.glMinor));
            accWall = accGpu = accCpu = 0; accCount = 0;
        }
    }

    if (haveGpuTimer) glDeleteQueries(2, timerQuery);
    level.reset();
    delete gBox; delete gPlane; delete gCylinder; delete gPyramid;
    return 0;
}
