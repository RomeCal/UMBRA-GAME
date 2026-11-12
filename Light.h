#pragma once
#include "CommonValues.h"
// Plain colour + intensities shared by every light type.  Owner: R2.
struct Light
{
    glm::vec3 colour{ 1.f, 1.f, 1.f };
    float ambientIntensity = 0.f;
    float diffuseIntensity = 1.f;
};
