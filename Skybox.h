#pragma once
#include "CommonValues.h"
#include "Shader.h"

// Owner: R3.  The sky is a procedurally generated cube map (no image files to lose).
// Face order = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i : +X, -X, +Y, -Y, +Z, -Z.
// Every texel's direction is computed from the OpenGL cube-map face table, so the six
// faces line up by construction.  The sun disc is NOT baked in: skybox.frag draws it
// from the live sun direction so it always matches the light that casts the shadows.
class Skybox
{
public:
    Skybox() : vao(0), vbo(0), ibo(0), cubeTex(0) {}
    ~Skybox();
    // faceFiles: six paths in the order +X -X +Y -Y +Z -Z.  If the list is empty, or ANY face cannot be
    // loaded, the sky is generated in code instead and the reason is printed (never a half-built cube map).
    bool Init(const std::vector<std::string>& faceFiles, int proceduralSize = 256);
    void Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& toSun);
private:
    Shader shader;
    GLuint vao, vbo, ibo, cubeTex;
    Skybox(const Skybox&); Skybox& operator=(const Skybox&);
};
