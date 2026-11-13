#include "Material.h"

void Material::Use(Shader& s) const
{
    s.SetFloat("material.specularIntensity", specularIntensity);
    s.SetFloat("material.shininess", shininess);
    s.SetVec3("material.emissive", emissive);
    if (texture) texture->Use(UNIT_DIFFUSE);
}
