#include "HUD.h"
#include <cctype>
#include <cstring>

// 5x7 glyphs, rows top to bottom ('1' = lit). '#' is a solid block, used for rectangles.
struct Glyph { char c; const char* rows[7]; };
static const Glyph GLYPHS[] = {
    {'A', {"01110","10001","10001","11111","10001","10001","10001"}},
    {'B', {"11110","10001","10001","11110","10001","10001","11110"}},
    {'C', {"01110","10001","10000","10000","10000","10001","01110"}},
    {'D', {"11110","10001","10001","10001","10001","10001","11110"}},
    {'E', {"11111","10000","10000","11110","10000","10000","11111"}},
    {'F', {"11111","10000","10000","11110","10000","10000","10000"}},
    {'G', {"01110","10001","10000","10111","10001","10001","01111"}},
    {'H', {"10001","10001","10001","11111","10001","10001","10001"}},
    {'I', {"01110","00100","00100","00100","00100","00100","01110"}},
    {'J', {"00111","00010","00010","00010","00010","10010","01100"}},
    {'K', {"10001","10010","10100","11000","10100","10010","10001"}},
    {'L', {"10000","10000","10000","10000","10000","10000","11111"}},
    {'M', {"10001","11011","10101","10101","10001","10001","10001"}},
    {'N', {"10001","11001","10101","10011","10001","10001","10001"}},
    {'O', {"01110","10001","10001","10001","10001","10001","01110"}},
    {'P', {"11110","10001","10001","11110","10000","10000","10000"}},
    {'Q', {"01110","10001","10001","10001","10101","10010","01101"}},
    {'R', {"11110","10001","10001","11110","10100","10010","10001"}},
    {'S', {"01111","10000","10000","01110","00001","00001","11110"}},
    {'T', {"11111","00100","00100","00100","00100","00100","00100"}},
    {'U', {"10001","10001","10001","10001","10001","10001","01110"}},
    {'V', {"10001","10001","10001","10001","10001","01010","00100"}},
    {'W', {"10001","10001","10001","10101","10101","11011","10001"}},
    {'X', {"10001","10001","01010","00100","01010","10001","10001"}},
    {'Y', {"10001","10001","01010","00100","00100","00100","00100"}},
    {'Z', {"11111","00001","00010","00100","01000","10000","11111"}},
    {'0', {"01110","10001","10011","10101","11001","10001","01110"}},
    {'1', {"00100","01100","00100","00100","00100","00100","01110"}},
    {'2', {"01110","10001","00001","00010","00100","01000","11111"}},
    {'3', {"11111","00010","00100","00010","00001","10001","01110"}},
    {'4', {"00010","00110","01010","10010","11111","00010","00010"}},
    {'5', {"11111","10000","11110","00001","00001","10001","01110"}},
    {'6', {"00110","01000","10000","11110","10001","10001","01110"}},
    {'7', {"11111","00001","00010","00100","01000","01000","01000"}},
    {'8', {"01110","10001","10001","01110","10001","10001","01110"}},
    {'9', {"01110","10001","10001","01111","00001","00010","01100"}},
    {'.', {"00000","00000","00000","00000","00000","01100","01100"}},
    {':', {"00000","01100","01100","00000","01100","01100","00000"}},
    {'-', {"00000","00000","00000","11111","00000","00000","00000"}},
    {'/', {"00001","00001","00010","00100","01000","10000","10000"}},
    {'!', {"00100","00100","00100","00100","00100","00000","00100"}},
    {',', {"00000","00000","00000","00000","01100","00100","01000"}},
    {'%', {"11000","11001","00010","00100","01000","10011","00011"}},
    {'(', {"00010","00100","01000","01000","01000","00100","00010"}},
    {')', {"01000","00100","00010","00010","00010","00100","01000"}},
    {'+', {"00000","00100","00100","11111","00100","00100","00000"}},
    {'?', {"01110","10001","00001","00010","00100","00000","00100"}},
    {'=', {"00000","00000","11111","00000","11111","00000","00000"}},
    {'\'', {"00100","00100","01000","00000","00000","00000","00000"}},
    {' ', {"00000","00000","00000","00000","00000","00000","00000"}},
    {'#', {"11111","11111","11111","11111","11111","11111","11111"}},
};
static const int GLYPH_COUNT = (int)(sizeof(GLYPHS) / sizeof(GLYPHS[0]));

int HUD::GlyphIndex(char c) const
{
    c = (char)std::toupper((unsigned char)c);
    for (int i = 0; i < GLYPH_COUNT; ++i) if (GLYPHS[i].c == c) return i;
    return 0;   // unknown -> space
}

bool HUD::Init()
{
    if (!shader.CreateFromFiles("Shaders/hud.vert", "", "Shaders/hud.frag")) return false;

    // the glyph strip: 6 pixels per glyph (5 + 1 gap), 7 pixels tall
    const int W = GLYPH_COUNT * 6, H = 7;
    std::vector<unsigned char> px((size_t)W * H * 4, 0);
    for (int g = 0; g < GLYPH_COUNT; ++g)
        for (int r = 0; r < 7; ++r)
            for (int c = 0; c < 5; ++c)
            {
                bool on = GLYPHS[g].rows[r][c] == '1';
                int y = 6 - r;     // first uploaded row is the BOTTOM of the glyph
                unsigned char* p = &px[((size_t)y * W + g * 6 + c) * 4];
                p[0] = p[1] = p[2] = 255; p[3] = on ? 255 : 0;
            }
    glGenTextures(1, &fontTex);
    glBindTexture(GL_TEXTURE_2D, fontTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &ibo);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLsizei stride = 8 * sizeof(GLfloat);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    verts.reserve(8 * 4 * 512);
    indices.reserve(6 * 512);
    return true;
}

void HUD::Begin(int w, int h)
{
    screenW = w; screenH = h;
    verts.clear(); indices.clear();
}

void HUD::Quad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, const glm::vec4& c)
{
    GLuint base = (GLuint)(verts.size() / 8);
    const float q[4][4] = { {x, y, u0, v0}, {x + w, y, u1, v0}, {x + w, y + h, u1, v1}, {x, y + h, u0, v1} };
    for (int i = 0; i < 4; ++i)
    {
        verts.push_back(q[i][0]); verts.push_back(q[i][1]);
        verts.push_back(q[i][2]); verts.push_back(q[i][3]);
        verts.push_back(c.r); verts.push_back(c.g); verts.push_back(c.b); verts.push_back(c.a);
    }
    indices.push_back(base); indices.push_back(base + 1); indices.push_back(base + 2);
    indices.push_back(base); indices.push_back(base + 2); indices.push_back(base + 3);
}

void HUD::Rect(float x, float y, float w, float h, const glm::vec4& colour)
{
    // sample the middle of the solid '#' glyph, whose alpha is 1 everywhere
    int g = GlyphIndex('#');
    float W = (float)(GLYPH_COUNT * 6);
    float u = ((float)g * 6.f + 2.5f) / W;
    Quad(x, y, w, h, u, 0.5f, u, 0.5f, colour);
}

void HUD::Text(const std::string& s, float x, float y, float scale, const glm::vec4& colour, bool dropShadow)
{
    if (dropShadow)   // a dark copy one 'pixel' down-right keeps white text readable over bright sand
        Text(s, x + scale * 0.7f, y - scale * 0.7f, scale, glm::vec4(0.f, 0.f, 0.f, 0.75f * colour.a), false);
    float W = (float)(GLYPH_COUNT * 6);
    for (size_t i = 0; i < s.size(); ++i)
    {
        int g = GlyphIndex(s[i]);
        if (s[i] != ' ')
        {
            float u0 = ((float)g * 6.f + 0.02f) / W;
            float u1 = ((float)g * 6.f + 4.98f) / W;
            Quad(x + (float)i * 6.f * scale, y, 5.f * scale, 7.f * scale, u0, 0.f, u1, 1.f, colour);
        }
    }
}

void HUD::End()
{
    if (indices.empty()) return;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.UseShader();
    // WHY ORTHO: a HUD has no depth and must not shrink with distance.  Pixel coordinates,
    // origin bottom-left: left=0, right=width, bottom=0, top=height, near=-1, far=1.
    shader.SetMat4("projection", glm::ortho(0.0f, (GLfloat)screenW, 0.0f, (GLfloat)screenH, -1.0f, 1.0f));
    shader.SetInt("hudTex", UNIT_DIFFUSE);
    glActiveTexture(GL_TEXTURE0 + UNIT_DIFFUSE);
    glBindTexture(GL_TEXTURE_2D, fontTex);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STREAM_DRAW);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

HUD::~HUD()
{
    if (ibo) glDeleteBuffers(1, &ibo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (fontTex) glDeleteTextures(1, &fontTex);
}
