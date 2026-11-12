#include "DirectionalLight.h"

DirectionalLight::DirectionalLight()
    : halfSize(20.f), nearPlane(0.1f), farPlane(100.f), resolution(2048),
      direction(0.f, -0.6f, 0.8f), lightTransform(1.f), shadow(nullptr)
{
    ambientIntensity = 0.25f; diffuseIntensity = 1.0f;
}

DirectionalLight::~DirectionalLight() { delete shadow; }

bool DirectionalLight::Init(glm::vec3 dir, glm::vec3 col, float ambient, float diffuse,
                            float half, float nearP, float farP, int res)
{
    direction = glm::normalize(dir);
    colour = col; ambientIntensity = ambient; diffuseIntensity = diffuse;
    halfSize = half; nearPlane = nearP; farPlane = farP; resolution = res;
    delete shadow;
    shadow = new ShadowMap();
    if (!shadow->Init(res, res)) return false;
    CalcLightTransform(glm::vec3(0.f), false);
    return true;
}

glm::mat4 DirectionalLight::CalcLightTransform(glm::vec3 centre, bool snap)
{
    glm::vec3 d  = glm::normalize(direction);
    glm::vec3 up = (std::fabs(d.y) > 0.99f) ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(0.f, 1.f, 0.f);
    glm::vec3 right  = glm::normalize(glm::cross(d, up));
    glm::vec3 lightUp = glm::cross(right, d);          // same basis lookAt() builds below

    if (snap)
    {
        // Round the box centre to whole texels measured along the light's own axes.
        // halfSize is an integer multiple of the texel size (res/2 texels), so the
        // texel grid lines stay put in world space as the box slides.
        float t = TexelSize();
        float x = glm::dot(centre, right);
        float y = glm::dot(centre, lightUp);
        float z = glm::dot(centre, d);
        x = std::floor(x / t + 0.5f) * t;
        y = std::floor(y / t + 0.5f) * t;
        centre = right * x + lightUp * y + d * z;
    }

    // A directional light has no position, but the box still needs a spot to look FROM
    // so that near..far brackets the scene.  Put the eye half a far-plane behind the centre.
    glm::vec3 eye = centre - d * (farPlane * 0.5f);
    glm::mat4 view = glm::lookAt(eye, centre, lightUp);
    glm::mat4 proj = glm::ortho(-halfSize, halfSize, -halfSize, halfSize, nearPlane, farPlane);
    lightTransform = proj * view;
    return lightTransform;
}
