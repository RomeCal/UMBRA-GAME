#pragma once
#include "CommonValues.h"

// Owner: R4 (movement is gameplay).  B4: every speed is multiplied by deltaTime.
class Camera
{
public:
    Camera();
    Camera(glm::vec3 startPosition, GLfloat startYaw, GLfloat startPitch, GLfloat turnSpeed);

    void mouseControl(GLfloat xChange, GLfloat yChange);
    void flyControl(const bool* keys, GLfloat deltaTime, GLfloat speed);   // debug free camera

    glm::mat4 calculateViewMatrix() const;
    glm::vec3 getPosition() const { return position; }
    void      setPosition(const glm::vec3& p) { position = p; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getFlatForward() const;
    glm::vec3 getFlatRight() const;
    void setAngles(GLfloat newYaw, GLfloat newPitch);

private:
    void update();
    glm::vec3 position, front, up, right, worldUp;
    GLfloat yaw, pitch, turnSpeed;
};
