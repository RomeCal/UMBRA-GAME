#include "PointLight.h"

PointLight::PointLight() : position(0.f), constant(1.f), linear(0.f), exponent(0.f), farPlane(40.f), resolution(1024), shadow(nullptr) {}
PointLight::~PointLight() { delete shadow; }

bool PointLight::Init(glm::vec3 pos, glm::vec3 col, float ambient, float diffuse,
                      float con, float lin, float exp, float farP, int res)
{
    position = pos; colour = col; ambientIntensity = ambient; diffuseIntensity = diffuse;
    constant = con; linear = lin; exponent = exp; farPlane = farP; resolution = res;
    delete shadow;
    shadow = new OmniShadowMap();
    return shadow->Init(res);
}

std::vector<glm::mat4> PointLight::CalcLightTransforms() const
{
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, farPlane);
    std::vector<glm::mat4> m;
    m.push_back(proj * glm::lookAt(position, position + glm::vec3( 1,  0,  0), glm::vec3(0, -1,  0)));
    m.push_back(proj * glm::lookAt(position, position + glm::vec3(-1,  0,  0), glm::vec3(0, -1,  0)));
    m.push_back(proj * glm::lookAt(position, position + glm::vec3( 0,  1,  0), glm::vec3(0,  0,  1)));
    m.push_back(proj * glm::lookAt(position, position + glm::vec3( 0, -1,  0), glm::vec3(0,  0, -1)));
    m.push_back(proj * glm::lookAt(position, position + glm::vec3( 0,  0,  1), glm::vec3(0, -1,  0)));
    m.push_back(proj * glm::lookAt(position, position + glm::vec3( 0,  0, -1), glm::vec3(0, -1,  0)));
    return m;
}
