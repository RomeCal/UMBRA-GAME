#include "SunProbe.h"
#include "DirectionalLight.h"

bool SunProbe::Init()
{
    if (!shader.CreateFromFiles("Shaders/sunprobe.vert", "", "Shaders/sunprobe.frag")) return false;

    glGenTextures(1, &colourTex);
    glBindTexture(GL_TEXTURE_2D, colourTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colourTex, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) { std::fprintf(stderr, "SunProbe framebuffer incomplete\n"); return false; }

    // one point, drawn with glDrawElements like everything else (B2)
    const GLfloat v[] = { 0.f, 0.f, 0.f };
    const GLuint  i[] = { 0 };
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

    glGenBuffers(2, pbo);
    const unsigned char zero[4] = { 0, 0, 0, 0 };   // first reads must mean 'in shade', not garbage
    for (int k = 0; k < 2; ++k)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[k]);
        glBufferData(GL_PIXEL_PACK_BUFFER, 4, zero, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return true;
}

void SunProbe::Query(const DirectionalLight& sun, const glm::vec3& feet, float radius,
                     float biasScale, float biasMin, bool shadowsOn, int restoreW, int restoreH)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1, 1);
    glDisable(GL_DEPTH_TEST);

    shader.UseShader();
    shader.SetInt("directionalShadowMap", UNIT_DIR_SHADOW);
    shader.SetMat4("directionalLightTransform", sun.GetTransform());
    shader.SetVec3("toSun", sun.ToSun());
    shader.SetVec3("samplePos", glm::vec3(feet.x, 0.02f, feet.z));   // the floor the player stands on
    shader.SetFloat("radius", radius * 0.75f);
    shader.SetFloat("dirBiasScale", biasScale);
    shader.SetFloat("dirBiasMin", biasMin);
    shader.SetInt("shadowsEnabled", shadowsOn ? 1 : 0);
    sun.GetShadowMap()->Read(GL_TEXTURE0 + UNIT_DIR_SHADOW);

    glBindVertexArray(vao);
    glDrawElements(GL_POINTS, 1, GL_UNSIGNED_INT, nullptr);

    // Start an asynchronous copy of this frame's pixel into one PBO, and read the other
    // PBO, which was filled last frame.
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[writeIndex]);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[1 - writeIndex]);
    const unsigned char* p = (const unsigned char*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if (p)
    {
        litFraction = (float)p[0] / 255.0f;
        outside = p[1] > 127;
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    writeIndex = 1 - writeIndex;

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, restoreW, restoreH);
}

SunProbe::~SunProbe()
{
    if (pbo[0]) glDeleteBuffers(2, pbo);
    if (ibo) glDeleteBuffers(1, &ibo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (colourTex) glDeleteTextures(1, &colourTex);
}
