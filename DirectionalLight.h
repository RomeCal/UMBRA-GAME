#pragma once
#include "Light.h"
#include "ShadowMap.h"

// The sun.  Owner: R2.  The shadow box half-size, near, far and resolution all come
// from the scene file (P3 is a one-line edit to a text file).
class DirectionalLight : public Light
{
public:
    DirectionalLight();
    ~DirectionalLight();
    bool Init(glm::vec3 dir, glm::vec3 col, float ambient, float diffuse,
              float halfSize, float nearPlane, float farPlane, int resolution);

    void SetDirection(const glm::vec3& d) { direction = glm::normalize(d); }
    glm::vec3 GetDirection() const { return direction; }
    glm::vec3 ToSun() const { return -direction; }

    // world units covered by one shadow-map texel:  2 * halfSize / resolution
    float TexelSize() const { return 2.0f * halfSize / (float)resolution; }

    // Builds the orthographic light-space matrix around 'centre'.  With snap = true the
    // centre is rounded to whole texels in light space (E1: stops edge crawl).
    glm::mat4 CalcLightTransform(glm::vec3 centre, bool snap);
    const glm::mat4& GetTransform() const { return lightTransform; }
    ShadowMap* GetShadowMap() const { return shadow; }

    float halfSize, nearPlane, farPlane;
    int   resolution;

private:
    glm::vec3 direction;
    glm::mat4 lightTransform;
    ShadowMap* shadow;
    DirectionalLight(const DirectionalLight&); DirectionalLight& operator=(const DirectionalLight&);
};
