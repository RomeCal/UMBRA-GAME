#pragma once
#include "CommonValues.h"
#include "Shader.h"

class DirectionalLight;

// Owner: R4 (mechanic) with R2 (shader).
// Reads the SUN SHADOW MAP at the player's feet on the GPU, into a 1x1 framebuffer, and
// reads the answer back through two pixel-buffer objects so the CPU never stalls
// (the result is one frame old, which is invisible at 60 fps).
class SunProbe
{
public:
    SunProbe() : fbo(0), colourTex(0), vao(0), vbo(0), ibo(0), writeIndex(0), litFraction(0.f), outside(false) { pbo[0] = pbo[1] = 0; }
    ~SunProbe();
    bool Init();
    // Run the probe for this frame.  Needs the sun shadow map to be up to date.
    void Query(const DirectionalLight& sun, const glm::vec3& feetPos, float radius,
               float biasScale, float biasMin, bool shadowsOn, int restoreW, int restoreH);
    float LitFraction() const { return litFraction; }     // 0 = fully in shade, 1 = fully in sun
    bool  OutsideBox()  const { return outside; }          // feet are outside the shadow box
private:
    Shader shader;
    GLuint fbo, colourTex, vao, vbo, ibo, pbo[2];
    int writeIndex;
    float litFraction;
    bool outside;
    SunProbe(const SunProbe&); SunProbe& operator=(const SunProbe&);
};
