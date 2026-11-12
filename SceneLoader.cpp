#include "SceneLoader.h"
#include <sstream>
#include <cstdlib>
#include <set>

namespace
{
    struct Reader
    {
        std::vector<std::string> t;
        size_t i = 1;                 // t[0] is the command word
        bool ok = true;
        std::string err;

        void fail(const std::string& m) { if (ok) { ok = false; err = m; } }
        float f(const char* what)
        {
            if (i >= t.size()) { fail(std::string("missing value for ") + what); return 0.f; }
            const std::string& s = t[i++];
            char* end = nullptr;
            float v = std::strtof(s.c_str(), &end);
            if (end == s.c_str() || *end != '\0') { fail(std::string("expected a number for ") + what + " but found '" + s + "'"); return 0.f; }
            return v;
        }
        int n(const char* what) { return (int)f(what); }
        std::string s(const char* what)
        {
            if (i >= t.size()) { fail(std::string("missing value for ") + what); return ""; }
            return t[i++];
        }
        glm::vec3 v3(const char* what) { float x = f(what), y = f(what), z = f(what); return glm::vec3(x, y, z); }
        bool more() const { return i < t.size(); }
    };

    bool IsPow2(int v) { return v > 0 && (v & (v - 1)) == 0; }
}

bool LoadScene(const std::string& path, SceneData& out)
{
    out = SceneData();
    out.path = path;
    std::string real = FindAsset(path);
    std::ifstream file(real.c_str());
    if (!file.good())
    {
        std::string m = "scene file '" + path + "' could not be opened";
        std::fprintf(stderr, "%s\n", m.c_str());
        out.messages.push_back(m); out.errorCount++;
        return false;
    }

    auto report = [&](int line, const std::string& what, bool isError) {
        std::string m = "scene file '" + path + "'" + (line > 0 ? " line " + std::to_string(line) : std::string(" (whole file)")) + ": " + what;
        std::fprintf(stderr, "%s%s\n", isError ? "ERROR  " : "WARNING ", m.c_str());
        out.messages.push_back(m);
        if (isError) out.errorCount++;
    };

    std::string text;
    int lineNo = 0;
    while (std::getline(file, text))
    {
        ++lineNo;
        size_t hash = text.find('#');
        if (hash != std::string::npos) text = text.substr(0, hash);
        std::istringstream ss(text);
        Reader r;
        std::string tok;
        while (ss >> tok) r.t.push_back(tok);
        if (r.t.empty()) continue;

        const std::string cmd = r.t[0];
        if (cmd == "level_name")
        {
            std::string name;
            for (size_t k = 1; k < r.t.size(); ++k) name += (k > 1 ? " " : "") + r.t[k];
            out.name = name;
        }
        else if (cmd == "next_level") { out.nextLevel = r.s("path"); }
        else if (cmd == "bounds")
        {
            float a = r.f("minX"), b = r.f("minZ"), c = r.f("maxX"), d = r.f("maxZ");
            if (!r.ok) { report(lineNo, r.err + " (bounds minX minZ maxX maxZ)", true); continue; }
            if (c <= a || d <= b) { report(lineNo, "bounds max must be larger than min", true); continue; }
            out.bounds = glm::vec4(a, b, c, d);
        }
        else if (cmd == "player")
        {
            float x = r.f("x"), z = r.f("z"); float yaw = r.more() ? r.f("yawDegrees") : 0.f;
            if (!r.ok) { report(lineNo, r.err + " (player x z [yaw])", true); continue; }
            out.playerStart = glm::vec2(x, z); out.playerYaw = yaw;
        }
        else if (cmd == "goal")
        {
            float x = r.f("x"), z = r.f("z"), rad = r.f("radius");
            if (!r.ok) { report(lineNo, r.err + " (goal x z radius)", true); continue; }
            if (rad <= 0.f) { report(lineNo, "goal radius must be positive", true); continue; }
            out.goal = glm::vec2(x, z); out.goalRadius = rad;
        }
        else if (cmd == "health")
        {
            float mx = r.f("maxHealth"), dr = r.f("drainPerSecondInSun"), rg = r.f("regenPerSecondInShade");
            if (!r.ok) { report(lineNo, r.err + " (health max drain regen)", true); continue; }
            if (mx <= 0.f) { report(lineNo, "maxHealth must be positive", true); continue; }
            out.maxHealth = mx; out.sunDrain = dr; out.shadeRegen = rg;
        }
        else if (cmd == "speed")
        {
            float w = r.f("walkSpeed"), sp = r.f("sprintMultiplier");
            if (!r.ok) { report(lineNo, r.err + " (speed walk sprintMultiplier)", true); continue; }
            out.walkSpeed = w; out.sprintMultiplier = sp;
        }
        else if (cmd == "eye_height")    { float v = r.f("height"); if (!r.ok) { report(lineNo, r.err, true); continue; } out.eyeHeight = v; }
        else if (cmd == "player_radius") { float v = r.f("radius"); if (!r.ok || v <= 0.f) { report(lineNo, r.ok ? "radius must be positive" : r.err, true); continue; } out.playerRadius = v; }
        else if (cmd == "sun")
        {
            glm::vec3 d = r.v3("direction"); glm::vec3 c = r.v3("colour");
            float amb = r.f("ambient"), dif = r.f("diffuse");
            if (!r.ok) { report(lineNo, r.err + " (sun dx dy dz  r g b  ambient diffuse)", true); continue; }
            if (glm::length(d) < 1e-4f) { report(lineNo, "sun direction must not be (0,0,0)", true); continue; }
            out.sun.dir = glm::normalize(d); out.sun.colour = c; out.sun.ambient = amb; out.sun.diffuse = dif;
            out.sun.defined = true;
        }
        else if (cmd == "sun_shadow")
        {
            float half = r.f("halfSize"), nr = r.f("near"), fr = r.f("far");
            int res = r.n("resolution"); float bs = r.f("biasScale"), bm = r.f("biasMin");
            if (!r.ok) { report(lineNo, r.err + " (sun_shadow halfSize near far resolution biasScale biasMin)", true); continue; }
            if (half <= 0.f) { report(lineNo, "halfSize must be positive", true); continue; }
            if (nr <= 0.f || fr <= nr) { report(lineNo, "need 0 < near < far", true); continue; }
            if (!IsPow2(res) || res > 8192) { report(lineNo, "resolution must be a power of two up to 8192, found " + std::to_string(res), true); continue; }
            out.sun.halfSize = half; out.sun.nearPlane = nr; out.sun.farPlane = fr;
            out.sun.resolution = res; out.sun.biasScale = bs; out.sun.biasMin = bm;
        }
        else if (cmd == "skybox")
        {
            std::vector<std::string> f;
            for (int k = 0; k < 6; ++k) f.push_back(r.s("face file"));
            if (!r.ok) { report(lineNo, r.err + " (skybox +X -X +Y -Y +Z -Z : six file paths in that order)", true); continue; }
            out.skyFaces = f;
        }
        else if (cmd == "hint")
        {
            std::string t;
            for (size_t k = 1; k < r.t.size(); ++k) t += (k > 1 ? " " : "") + r.t[k];
            if (t.empty()) { report(lineNo, "hint needs some text", true); continue; }
            out.hints.push_back(t);
        }
        else if (cmd == "stamina")
        {
            float mx = r.f("max"), dr = r.f("drainPerSecondSprinting"), rg = r.f("regenPerSecond");
            if (!r.ok) { report(lineNo, r.err + " (stamina max drain regen)", true); continue; }
            if (mx <= 0.f || dr < 0.f || rg < 0.f) { report(lineNo, "stamina values must be positive", true); continue; }
            out.maxStamina = mx; out.staminaDrain = dr; out.staminaRegen = rg;
        }
        else if (cmd == "fog")
        {
            glm::vec3 c = r.v3("colour"); float d = r.f("density");
            if (!r.ok) { report(lineNo, r.err + " (fog r g b density)", true); continue; }
            if (d < 0.f) { report(lineNo, "fog density must not be negative", true); continue; }
            out.fogColour = c; out.fogDensity = d;
        }
        else if (cmd == "par_time")
        {
            float v = r.f("seconds");
            if (!r.ok || v <= 0.f) { report(lineNo, r.ok ? "par_time must be positive" : r.err + " (par_time seconds)", true); continue; }
            out.parTime = v;
        }
        else if (cmd == "omni_bias")
        {
            float v = r.f("worldUnits");
            if (!r.ok || v < 0.f) { report(lineNo, r.ok ? "omni_bias must not be negative" : r.err + " (omni_bias worldUnits)", true); continue; }
            out.omniBias = v;
        }
        else if (cmd == "sun_center")
        {
            glm::vec3 c = r.v3("centre");
            if (!r.ok) { report(lineNo, r.err + " (sun_center x y z)", true); continue; }
            out.sun.centre = c;
        }
        else if (cmd == "sun_sweep")
        {
            float amp = r.f("amplitudeDegrees"), per = r.f("periodSeconds");
            if (!r.ok) { report(lineNo, r.err + " (sun_sweep amplitudeDeg periodSeconds)", true); continue; }
            if (per <= 0.f) { report(lineNo, "period must be positive", true); continue; }
            out.sun.sweepAmplitudeDeg = amp; out.sun.sweepPeriodSec = per;
        }
        else if (cmd == "sun_follow")
        {
            std::string on = r.s("on|off"); float ahead = r.f("distanceAhead"); std::string snap = r.s("snap on|off");
            if (!r.ok) { report(lineNo, r.err + " (sun_follow on|off distanceAhead on|off)", true); continue; }
            out.sun.follow = (on == "on"); out.sun.followAhead = ahead; out.sun.snap = (snap == "on");
        }
        else if (cmd == "point")
        {
            PointDef p;
            p.pos = r.v3("position"); p.colour = r.v3("colour");
            p.ambient = r.f("ambient"); p.diffuse = r.f("diffuse");
            p.constant = r.f("constant"); p.linear = r.f("linear"); p.exponent = r.f("exponent");
            p.farPlane = r.f("farPlane"); p.resolution = r.n("resolution"); p.line = lineNo;
            if (!r.ok) { report(lineNo, r.err + " (point x y z  r g b  amb diff  con lin exp  farPlane res)", true); continue; }
            if (p.farPlane <= 0.1f) { report(lineNo, "farPlane must be larger than 0.1", true); continue; }
            if (!IsPow2(p.resolution) || p.resolution > 4096) { report(lineNo, "point light resolution must be a power of two up to 4096", true); continue; }
            if (p.constant <= 0.f && p.linear <= 0.f && p.exponent <= 0.f) { report(lineNo, "attenuation terms cannot all be zero", true); continue; }
            if ((int)out.points.size() >= MAX_POINT_LIGHTS) { report(lineNo, "more than 3 point lights: this one is ignored", false); continue; }
            out.points.push_back(p);
        }
        else if (cmd == "spot")
        {
            SpotDef s;
            s.pos = r.v3("position"); s.dir = r.v3("direction"); s.colour = r.v3("colour");
            s.ambient = r.f("ambient"); s.diffuse = r.f("diffuse");
            s.constant = r.f("constant"); s.linear = r.f("linear"); s.exponent = r.f("exponent");
            s.edgeDeg = r.f("edgeDegrees"); s.line = lineNo;
            while (r.more()) { std::string fl = r.s("flag"); if (fl == "camera") s.attachToCamera = true; else report(lineNo, "unknown spot flag '" + fl + "'", false); }
            if (!r.ok) { report(lineNo, r.err + " (spot x y z  dx dy dz  r g b  amb diff  con lin exp  edgeDeg [camera])", true); continue; }
            if (s.edgeDeg <= 0.f || s.edgeDeg >= 89.f) { report(lineNo, "edgeDegrees must be between 0 and 89", true); continue; }
            if (glm::length(s.dir) < 1e-4f) { report(lineNo, "spot direction must not be (0,0,0)", true); continue; }
            if ((int)out.spots.size() >= MAX_SPOT_LIGHTS) { report(lineNo, "more than 3 spot lights: this one is ignored", false); continue; }
            out.spots.push_back(s);
        }
        else if (cmd == "texture")
        {
            TextureDef t; t.name = r.s("name"); std::string kind = r.s("procedural|file"); t.line = lineNo;
            if (kind == "file")
            {
                t.isFile = true; t.path = r.s("path");
                if (r.ok)
                {   // a missing file is reported here with its line number and survived (magenta placeholder), never a silent black texture
                    std::ifstream probe(FindAsset(t.path).c_str(), std::ios::binary);
                    if (!probe.good()) { t.missing = true; report(lineNo, "texture '" + t.name + "': file '" + t.path + "' was not found -> the magenta placeholder will be used", true); }
                }
            }
            else if (kind == "procedural")
            {
                t.type = r.s("type"); t.c1 = r.v3("colour1"); t.c2 = r.v3("colour2"); t.size = r.n("size");
                static const std::set<std::string> known = { "plain","checker","brick","stripes","noise","stone","grid" };
                if (r.ok && !known.count(t.type)) { report(lineNo, "unknown procedural texture type '" + t.type + "' (plain checker brick stripes noise stone grid)", true); continue; }
                if (r.ok && (t.size < 8 || t.size > 2048)) { report(lineNo, "texture size must be 8..2048", true); continue; }
            }
            else if (r.ok) { report(lineNo, "texture kind must be 'procedural' or 'file', found '" + kind + "'", true); continue; }
            if (!r.ok) { report(lineNo, r.err + " (texture name procedural type r g b r g b size | texture name file path)", true); continue; }
            out.textures.push_back(t);
        }
        else if (cmd == "material")
        {
            MaterialDef m; m.name = r.s("name"); m.texture = r.s("texture");
            m.specular = r.f("specularIntensity"); m.shininess = r.f("shininess"); m.line = lineNo;
            if (r.more()) { std::string kw = r.s("emissive"); if (kw == "emissive") m.emissive = r.v3("emissive colour"); else r.fail("unexpected '" + kw + "' (expected the word 'emissive')"); }
            if (!r.ok) { report(lineNo, r.err + " (material name texture specular shininess [emissive r g b])", true); continue; }
            if (m.shininess < 1.f) { report(lineNo, "shininess must be at least 1", true); continue; }
            out.materials.push_back(m);
        }
        else if (cmd == "object")
        {
            ObjectDef o; o.shape = r.s("shape"); o.material = r.s("material");
            o.pos = r.v3("position"); o.scale = r.v3("scale"); o.line = lineNo;
            while (r.more())
            {
                std::string fl = r.s("flag");
                if (fl == "solid") o.solid = true;
                else if (fl == "movable") { o.movable = true; o.solid = true; }
                else if (fl == "nocast") o.castShadow = false;
                else if (fl.compare(0, 5, "tile=") == 0)
                {
                    float a = 1.f, b = 1.f;
                    std::string v = fl.substr(5);
                    size_t comma = v.find(',');
                    char* e1 = nullptr; a = std::strtof(v.substr(0, comma).c_str(), &e1);
                    b = (comma == std::string::npos) ? a : std::strtof(v.substr(comma + 1).c_str(), nullptr);
                    if (a <= 0.f || b <= 0.f) r.fail("tile values must be positive in '" + fl + "'");
                    o.tile = glm::vec2(a, b);
                }
                else r.fail("unknown object flag '" + fl + "' (solid movable nocast tile=N or tile=A,B)");
            }
            if (!r.ok) { report(lineNo, r.err + " (object shape material  x y z  sx sy sz  [flags])", true); continue; }
            if (o.shape != "box" && o.shape != "plane" && o.shape != "cylinder" && o.shape != "pyramid")
            { report(lineNo, "unknown shape '" + o.shape + "' (box plane cylinder pyramid)", true); continue; }
            if (o.scale.x <= 0.f || o.scale.y <= 0.f || o.scale.z <= 0.f) { report(lineNo, "scale values must be positive", true); continue; }
            out.objects.push_back(o);
        }
        else
        {
            report(lineNo, "unknown command '" + cmd + "'", true);
        }
    }

    // ---- cross checks: a missing reference is reported and survived, never a silent black texture ----
    if (!out.sun.defined) report(0, "no 'sun' line: using a default sun", false);

    std::set<std::string> texNames, matNames;
    for (const TextureDef& t : out.textures) texNames.insert(t.name);
    for (MaterialDef& m : out.materials)
    {
        if (!texNames.count(m.texture))
        {
            report(m.line, "material '" + m.name + "' uses unknown texture '" + m.texture + "': using the magenta placeholder", true);
            m.texture = "__missing";
        }
        matNames.insert(m.name);
    }
    for (ObjectDef& o : out.objects)
        if (!matNames.count(o.material))
        {
            report(o.line, "object uses unknown material '" + o.material + "': using the default material", true);
            o.material = "__default";
        }

    auto insideSolid = [&](glm::vec2 p, float pad, std::string& who) {
        for (const ObjectDef& o : out.objects)
        {
            if (!o.solid) continue;
            if (p.x > o.pos.x - o.scale.x * 0.5f - pad && p.x < o.pos.x + o.scale.x * 0.5f + pad &&
                p.y > o.pos.z - o.scale.z * 0.5f - pad && p.y < o.pos.z + o.scale.z * 0.5f + pad)
            { who = o.shape + " on line " + std::to_string(o.line); return true; }
        }
        return false;
    };
    { std::string who;
      if (insideSolid(out.playerStart, out.playerRadius, who)) report(0, "the player start is inside a solid object (" + who + ")", true);
      if (insideSolid(out.goal, 0.f, who)) report(0, "the goal is inside a solid object (" + who + ")", true);
      if (out.playerStart.x < out.bounds.x || out.playerStart.x > out.bounds.z || out.playerStart.y < out.bounds.y || out.playerStart.y > out.bounds.w)
          report(0, "the player start is outside 'bounds'", true);
    }
    bool anyMovable = false;
    for (const ObjectDef& o : out.objects) anyMovable = anyMovable || o.movable;
    if (!anyMovable) report(0, "no 'movable' object: the mechanic needs one piece of geometry the player can move", false);

    if (out.objects.empty())
    {
        report(0, "the scene contains no objects", true);
        return false;
    }
    return true;
}
