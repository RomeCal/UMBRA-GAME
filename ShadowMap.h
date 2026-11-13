#pragma once
#include "CommonValues.h"
// 2D depth map for the directional light.  Owner: R2.
class ShadowMap
{
public:
    ShadowMap() : fbo(0), tex(0), width(0), height(0) {}
    ~ShadowMap();
    bool Init(GLsizei w, GLsizei h);
    void Write() const { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
    void Read(GLenum textureUnit) const { glActiveTexture(textureUnit); glBindTexture(GL_TEXTURE_2D, tex); }
    GLsizei GetWidth() const { return width; }
    GLsizei GetHeight() const { return height; }
    GLuint GetTexture() const { return tex; }
private:
    GLuint fbo, tex; GLsizei width, height;
    ShadowMap(const ShadowMap&); ShadowMap& operator=(const ShadowMap&);
};
