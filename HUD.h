#pragma once
#include "CommonValues.h"
#include "Shader.h"

// Owner: R4.  Flat 2D drawing with glm::ortho in pixel coordinates (origin bottom-left).
// Text without a font library: a "glyph strip" texture (5x7 pixel letters side by side),
// one textured quad per character, texture coordinates shifted to pick the letter.
// Everything is batched into one dynamic vertex buffer and drawn with ONE glDrawElements.
class HUD
{
public:
    HUD() : vao(0), vbo(0), ibo(0), fontTex(0), screenW(1), screenH(1) {}
    ~HUD();
    bool Init();
    void Begin(int width, int height);
    void Rect(float x, float y, float w, float h, const glm::vec4& colour);
    void Text(const std::string& s, float x, float y, float scale, const glm::vec4& colour, bool dropShadow = true);
    static float TextHeight(float scale) { return 7.f * scale; }
    float TextWidth(const std::string& s, float scale) const { return s.empty() ? 0.f : (float)s.size() * 6.f * scale - scale; }
    void End();
private:
    int GlyphIndex(char c) const;
    void Quad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, const glm::vec4& c);
    Shader shader;
    GLuint vao, vbo, ibo, fontTex;
    int screenW, screenH;
    std::vector<GLfloat> verts;
    std::vector<GLuint> indices;
    HUD(const HUD&); HUD& operator=(const HUD&);
};
