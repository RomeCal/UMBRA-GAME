#pragma once
#include "CommonValues.h"
// Cube depth map for point lights.  Owner: R2.
// glFramebufferTexture attaches the WHOLE cube map (all six layers), so the geometry
// shader can route triangles to faces with gl_Layer: ONE draw pass per light, six
// times the rasterised triangles.
class OmniShadowMap
{
public:
    OmniShadowMap() : fbo(0), tex(0), resolution(0) {}
    ~OmniShadowMap();
    bool Init(GLsizei res);
    void Write() const { glBindFramebuffer(GL_FRAMEBUFFER, fbo); }
    void Read(GLenum textureUnit) const { glActiveTexture(textureUnit); glBindTexture(GL_TEXTURE_CUBE_MAP, tex); }
    GLsizei GetResolution() const { return resolution; }
private:
    GLuint fbo, tex; GLsizei resolution;
    OmniShadowMap(const OmniShadowMap&); OmniShadowMap& operator=(const OmniShadowMap&);
};
