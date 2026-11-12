#pragma once
#include "Light.h"
#include "OmniShadowMap.h"

// A torch.  Casts shadows in all six directions.  Owner: R2.
class PointLight : public Light
{
public:
    PointLight();
    ~PointLight();
    bool Init(glm::vec3 pos, glm::vec3 col, float ambient, float diffuse,
              float con, float lin, float exp, float farP, int res);
    std::vector<glm::mat4> CalcLightTransforms() const;   // six face matrices, +X -X +Y -Y +Z -Z
    OmniShadowMap* GetShadowMap() const { return shadow; }

    glm::vec3 position;
    float constant, linear, exponent, farPlane;
    int resolution;
private:
    OmniShadowMap* shadow;
    PointLight(const PointLight&); PointLight& operator=(const PointLight&);
};
