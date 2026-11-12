#pragma once
#include "Light.h"

// A cone of light with a soft edge.  No shadow map (only the sun and the two or three
// point lights cast shadows), which keeps the frame cost down.  Owner: R2.
class SpotLight : public Light
{
public:
    SpotLight() : position(0.f), direction(0.f, -1.f, 0.f), constant(1.f), linear(0.f), exponent(0.f),
                  edgeDegrees(20.f), cosEdge(std::cos(20.f * TO_RADIANS)), attachToCamera(false), enabled(true) {}
    void SetEdge(float deg) { edgeDegrees = deg; cosEdge = std::cos(deg * TO_RADIANS); }

    glm::vec3 position, direction;
    float constant, linear, exponent;
    float edgeDegrees, cosEdge;
    bool attachToCamera;   // the player's flashlight
    bool enabled;
};
