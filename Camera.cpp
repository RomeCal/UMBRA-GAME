#include "Camera.h"

Camera::Camera() : position(0.f), front(0.f, 0.f, -1.f), up(0.f, 1.f, 0.f), right(1.f, 0.f, 0.f),
                   worldUp(0.f, 1.f, 0.f), yaw(-90.f), pitch(0.f), turnSpeed(0.15f)
{
    update();
}

Camera::Camera(glm::vec3 startPosition, GLfloat startYaw, GLfloat startPitch, GLfloat startTurnSpeed)
    : position(startPosition), front(0.f, 0.f, -1.f), up(0.f, 1.f, 0.f), right(1.f, 0.f, 0.f),
      worldUp(0.f, 1.f, 0.f), yaw(startYaw), pitch(startPitch), turnSpeed(startTurnSpeed)
{
    update();
}

void Camera::mouseControl(GLfloat xChange, GLfloat yChange)
{
    yaw   += xChange * turnSpeed;
    pitch += yChange * turnSpeed;
    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    update();
}

void Camera::flyControl(const bool* keys, GLfloat dt, GLfloat speed)
{
    GLfloat v = speed * dt;
    if (keys[GLFW_KEY_LEFT_SHIFT]) v *= 3.0f;
    if (keys[GLFW_KEY_W]) position += front * v;
    if (keys[GLFW_KEY_S]) position -= front * v;
    if (keys[GLFW_KEY_A]) position -= right * v;
    if (keys[GLFW_KEY_D]) position += right * v;
    if (keys[GLFW_KEY_SPACE])        position += worldUp * v;
    if (keys[GLFW_KEY_LEFT_CONTROL]) position -= worldUp * v;
}

glm::mat4 Camera::calculateViewMatrix() const { return glm::lookAt(position, position + front, up); }

glm::vec3 Camera::getFlatForward() const
{
    glm::vec3 f(front.x, 0.f, front.z);
    float len = glm::length(f);
    return len > 1e-5f ? f / len : glm::vec3(0.f, 0.f, -1.f);
}

glm::vec3 Camera::getFlatRight() const
{
    glm::vec3 f = getFlatForward();
    return glm::vec3(-f.z, 0.f, f.x);
}

void Camera::setAngles(GLfloat newYaw, GLfloat newPitch) { yaw = newYaw; pitch = newPitch; update(); }

void Camera::update()
{
    front.x = std::cos(yaw * TO_RADIANS) * std::cos(pitch * TO_RADIANS);
    front.y = std::sin(pitch * TO_RADIANS);
    front.z = std::sin(yaw * TO_RADIANS) * std::cos(pitch * TO_RADIANS);
    front = glm::normalize(front);
    right = glm::normalize(glm::cross(front, worldUp));
    up    = glm::normalize(glm::cross(right, front));
}
