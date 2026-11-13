#include "Skybox.h"
#include "Texture.h"

static glm::vec3 SkyColour(glm::vec3 d)
{
    const glm::vec3 horizon(0.88f, 0.78f, 0.62f), zenith(0.20f, 0.42f, 0.80f), ground(0.30f, 0.27f, 0.24f);
    glm::vec3 col;
    if (d.y >= 0.f) col = glm::mix(horizon, zenith, std::pow(d.y, 0.5f));
    else            col = glm::mix(horizon, ground, std::min(1.f, -d.y * 3.f));
    if (d.y > 0.05f)
    {
        float k = 1.f / (d.y + 0.2f);
        float n = Fbm2D(d.x * k * 4.5f + 50.f, d.z * k * 4.5f + 50.f);
        float c = glm::smoothstep(0.55f, 0.80f, n) * glm::clamp(d.y * 4.f, 0.f, 1.f);
        col = glm::mix(col, glm::vec3(1.f), c * 0.7f);
    }
    return col;
}

bool Skybox::Init(const std::vector<std::string>& faceFiles, int N)
{
    if (!shader.CreateFromFiles("Shaders/skybox.vert", "", "Shaders/skybox.frag")) return false;

    glGenTextures(1, &cubeTex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // ---- try the six files first; accept them only if ALL six load and are the same square size ----
    static const char* faceNames[6] = { "+X (right)", "-X (left)", "+Y (up)", "-Y (down)", "+Z (front)", "-Z (back)" };
    std::vector<std::vector<unsigned char> > faces(6);
    bool fromFiles = (faceFiles.size() == 6);
    int fw = 0;
    for (int i = 0; i < 6 && fromFiles; ++i)
    {
        int w = 0, h = 0; std::string err;
        if (!Texture::DecodeTGA(faceFiles[i], faces[i], w, h, false, err))
        {
            std::fprintf(stderr, "ERROR  sky face %s '%s': %s -> using the generated sky instead\n", faceNames[i], faceFiles[i].c_str(), err.c_str());
            fromFiles = false;
        }
        else if (w != h || (i > 0 && w != fw))
        {
            std::fprintf(stderr, "ERROR  sky face %s '%s' is %dx%d: all six faces must be the same square size -> using the generated sky instead\n", faceNames[i], faceFiles[i].c_str(), w, h);
            fromFiles = false;
        }
        else fw = w;
    }

    if (fromFiles)
    {
        std::printf("Sky cube map from files (face -> GL target, size %dx%d):\n", fw, fw);
        for (int i = 0; i < 6; ++i)
        {
            std::printf("  GL_TEXTURE_CUBE_MAP_POSITIVE_X + %d  %-11s <- %s\n", i, faceNames[i], faceFiles[i].c_str());
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, fw, fw, 0, GL_RGBA, GL_UNSIGNED_BYTE, faces[i].data());
        }
    }
    else
    {
        std::vector<unsigned char> px((size_t)N * N * 4);
        for (int face = 0; face < 6; ++face)
        {
            for (int y = 0; y < N; ++y)
                for (int x = 0; x < N; ++x)
                {
                    float sc = 2.f * ((float)x + 0.5f) / N - 1.f;   // OpenGL spec cube-map table:
                    float tc = 2.f * ((float)y + 0.5f) / N - 1.f;   // (sc, tc) -> direction per face
                    glm::vec3 d;
                    switch (face)
                    {
                    case 0: d = glm::vec3( 1.f, -tc, -sc); break;   // +X
                    case 1: d = glm::vec3(-1.f, -tc,  sc); break;   // -X
                    case 2: d = glm::vec3( sc,  1.f,  tc); break;   // +Y
                    case 3: d = glm::vec3( sc, -1.f, -tc); break;   // -Y
                    case 4: d = glm::vec3( sc, -tc,  1.f); break;   // +Z
                    default: d = glm::vec3(-sc, -tc, -1.f); break;  // -Z
                    }
                    glm::vec3 c = SkyColour(glm::normalize(d));
                    unsigned char* p = &px[((size_t)y * N + x) * 4];
                    p[0] = (unsigned char)(glm::clamp(c.r, 0.f, 1.f) * 255.f);
                    p[1] = (unsigned char)(glm::clamp(c.g, 0.f, 1.f) * 255.f);
                    p[2] = (unsigned char)(glm::clamp(c.b, 0.f, 1.f) * 255.f);
                    p[3] = 255;
                }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA8, N, N, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        }
        std::printf("Sky cube map generated in code (%dx%d per face)\n", N, N);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // The sky really can use 8 corners: skybox.frag uses the position as the direction and ignores normals.
    const GLfloat v[] = { -1,-1,-1,  1,-1,-1,  1,1,-1,  -1,1,-1,  -1,-1,1,  1,-1,1,  1,1,1,  -1,1,1 };
    const GLuint  i[] = { 0,1,2, 2,3,0,  4,5,6, 6,7,4,  0,3,7, 7,4,0,  1,2,6, 6,5,1,  0,1,5, 5,4,0,  3,2,6, 6,7,3 };
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(i), i, GL_STATIC_DRAW);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return true;
}

void Skybox::Draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& toSun)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    shader.UseShader();
    shader.SetMat4("view", glm::mat4(glm::mat3(view)));
    shader.SetMat4("projection", projection);
    shader.SetVec3("sunDir", toSun);
    glActiveTexture(GL_TEXTURE0 + UNIT_SKY);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

Skybox::~Skybox()
{
    if (ibo) glDeleteBuffers(1, &ibo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (cubeTex) glDeleteTextures(1, &cubeTex);
}
