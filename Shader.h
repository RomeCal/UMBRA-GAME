#pragma once
#include "CommonValues.h"
#include <unordered_map>

class DirectionalLight;
class PointLight;
class SpotLight;

// Owner: R2.  Compiles/links GLSL files and sets uniforms (names are cached).
class Shader
{
public:
    Shader();
    ~Shader();

    // gs may be an empty string.  Returns false (and prints the log) on any error
    bool CreateFromFiles(const std::string& vs, const std::string& gs, const std::string& fs);
    void UseShader() const { glUseProgram(program); }
    GLuint GetID() const { return program; }

    GLint Loc(const std::string& name);
    void SetInt(const std::string& n, int v)               { glUniform1i(Loc(n), v); }
    void SetFloat(const std::string& n, float v)           { glUniform1f(Loc(n), v); }
    void SetVec2(const std::string& n, const glm::vec2& v) { glUniform2fv(Loc(n), 1, glm::value_ptr(v)); }
    void SetVec3(const std::string& n, const glm::vec3& v) { glUniform3fv(Loc(n), 1, glm::value_ptr(v)); }
    void SetVec4(const std::string& n, const glm::vec4& v) { glUniform4fv(Loc(n), 1, glm::value_ptr(v)); }
    void SetMat3(const std::string& n, const glm::mat3& m) { glUniformMatrix3fv(Loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void SetMat4(const std::string& n, const glm::mat4& m) { glUniformMatrix4fv(Loc(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void SetMat4Array(const std::string& n, const std::vector<glm::mat4>& m)
    { glUniformMatrix4fv(Loc(n), (GLsizei)m.size(), GL_FALSE, glm::value_ptr(m[0])); }

    // Light uniforms for shader.frag (the structs in the shader mirror these names).
    void SetDirectionalLight(const DirectionalLight& l);
    void SetPointLights(const std::vector<PointLight*>& lights, int firstUnit);
    void SetSpotLights(const std::vector<SpotLight*>& lights);

    // Used when the machine refused OpenGL 4.6: rewrites the first "#version" line.
    static void SetVersionOverride(const std::string& line);

private:
    GLuint program;
    std::unordered_map<std::string, GLint> cache;
    Shader(const Shader&); Shader& operator=(const Shader&);
};
