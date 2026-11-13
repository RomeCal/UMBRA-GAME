#include "Shader.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include <sstream>

static std::string& VersionOverride() { static std::string s; return s; }
void Shader::SetVersionOverride(const std::string& line) { VersionOverride() = line; }

Shader::Shader() : program(0) {}
Shader::~Shader() { if (program) glDeleteProgram(program); }

static bool ReadSource(const std::string& rel, std::string& out)
{
    std::string path = FindAsset(rel);
    std::ifstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f.good()) { std::fprintf(stderr, "Shader file not found: %s\n", rel.c_str()); return false; }
    std::stringstream ss; ss << f.rdbuf();
    out = ss.str();
    const std::string& ov = VersionOverride();
    if (!ov.empty() && out.compare(0, 8, "#version") == 0)
    {
        size_t eol = out.find('\n');
        out = ov + (eol == std::string::npos ? std::string() : out.substr(eol));
    }
    return true;
}

static GLuint CompileStage(GLenum type, const std::string& rel)
{
    std::string src;
    if (!ReadSource(rel, src)) return 0;
    GLuint s = glCreateShader(type);
    const char* p = src.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader compile error in %s:\n%s\n", rel.c_str(), log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool Shader::CreateFromFiles(const std::string& vs, const std::string& gs, const std::string& fs)
{
    GLuint v = CompileStage(GL_VERTEX_SHADER, vs);
    GLuint g = gs.empty() ? 0 : CompileStage(GL_GEOMETRY_SHADER, gs);
    GLuint f = CompileStage(GL_FRAGMENT_SHADER, fs);
    if (!v || !f || (!gs.empty() && !g)) { glDeleteShader(v); glDeleteShader(g); glDeleteShader(f); return false; }

    program = glCreateProgram();
    glAttachShader(program, v);
    if (g) glAttachShader(program, g);
    glAttachShader(program, f);
    glLinkProgram(program);
    GLint ok = 0; glGetProgramiv(program, GL_LINK_STATUS, &ok);
    glDeleteShader(v); if (g) glDeleteShader(g); glDeleteShader(f);
    if (!ok)
    {
        char log[2048]; glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Shader link error (%s):\n%s\n", vs.c_str(), log);
        glDeleteProgram(program); program = 0;
        return false;
    }
    cache.clear();
    return true;
}

GLint Shader::Loc(const std::string& name)
{
    auto it = cache.find(name);
    if (it != cache.end()) return it->second;
    GLint l = glGetUniformLocation(program, name.c_str());
    cache[name] = l;   // -1 is cached too, and silently ignored by glUniform*
    return l;
}

void Shader::SetDirectionalLight(const DirectionalLight& l)
{
    SetVec3("directionalLight.base.colour", l.colour);
    SetFloat("directionalLight.base.ambientIntensity", l.ambientIntensity);
    SetFloat("directionalLight.base.diffuseIntensity", l.diffuseIntensity);
    SetVec3("directionalLight.direction", l.GetDirection());
}

void Shader::SetPointLights(const std::vector<PointLight*>& lights, int firstUnit)
{
    int count = (int)std::min<size_t>(lights.size(), MAX_POINT_LIGHTS);
    SetInt("pointLightCount", count);
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i)
    {
        std::string b = "pointLights[" + std::to_string(i) + "].";
        // units are always assigned so no sampler is left on the default unit 0
        SetInt("omniShadowMaps[" + std::to_string(i) + "]", firstUnit + i);
        if (i >= count) continue;
        const PointLight& p = *lights[i];
        SetVec3 (b + "base.colour", p.colour);
        SetFloat(b + "base.ambientIntensity", p.ambientIntensity);
        SetFloat(b + "base.diffuseIntensity", p.diffuseIntensity);
        SetVec3 (b + "position", p.position);
        SetFloat(b + "constant", p.constant);
        SetFloat(b + "linear", p.linear);
        SetFloat(b + "exponent", p.exponent);
        SetFloat("omniFarPlane[" + std::to_string(i) + "]", p.farPlane);
    }
}

void Shader::SetSpotLights(const std::vector<SpotLight*>& lights)
{
    int n = 0;
    for (size_t i = 0; i < lights.size() && n < MAX_SPOT_LIGHTS; ++i)
    {
        const SpotLight& s = *lights[i];
        if (!s.enabled) continue;
        std::string b = "spotLights[" + std::to_string(n) + "].";
        SetVec3 (b + "base.base.colour", s.colour);
        SetFloat(b + "base.base.ambientIntensity", s.ambientIntensity);
        SetFloat(b + "base.base.diffuseIntensity", s.diffuseIntensity);
        SetVec3 (b + "base.position", s.position);
        SetFloat(b + "base.constant", s.constant);
        SetFloat(b + "base.linear", s.linear);
        SetFloat(b + "base.exponent", s.exponent);
        SetVec3 (b + "direction", glm::normalize(s.direction));
        SetFloat(b + "edge", s.cosEdge);
        ++n;
    }
    SetInt("spotLightCount", n);
}
