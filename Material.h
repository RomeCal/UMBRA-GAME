#pragma once
#include "Texture.h"
#include "Shader.h"

// Owner: R3.  Texture + the two Phong numbers + an optional glow colour.
struct Material
{
    std::string name;
    Texture* texture = nullptr;       // owned by the level, not by the material
    float specularIntensity = 0.2f;
    float shininess = 16.f;
    glm::vec3 emissive{ 0.f };

    void Use(Shader& s) const;
};
