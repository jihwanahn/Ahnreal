#include "Camera.h"

namespace AhnrealEngine {

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position), worldUp(up), yaw(yaw), pitch(pitch),
      movementSpeed(DEFAULT_SPEED), mouseSensitivity(DEFAULT_SENSITIVITY),
      zoom(DEFAULT_ZOOM), nearPlane(DEFAULT_NEAR), farPlane(DEFAULT_FAR),
      front(glm::vec3(0.0f, 0.0f, -1.0f)) {
  updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
  if (mode == CameraMode::Orbit) {
    return glm::lookAt(position, orbitTarget, worldUp);
  }
  return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
  glm::mat4 proj =
      glm::perspective(glm::radians(zoom), aspectRatio, nearPlane, farPlane);
  // Vulkan uses inverted Y compared to OpenGL
  proj[1][1] *= -1;
  return proj;
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
  if (mode == CameraMode::Orbit) {
    // In orbit mode, keyboard controls orbit distance
    float velocity = movementSpeed * deltaTime;
    if (direction == CameraMovement::Forward) {
      orbitDistance -= velocity;
      orbitDistance = glm::max(orbitDistance, 0.5f);
    }
    if (direction == CameraMovement::Backward) {
      orbitDistance += velocity;
    }
    updateOrbitPosition();
    return;
  }

  // Free camera movement
  float velocity = movementSpeed * deltaTime;

  if (direction == CameraMovement::Forward)
    position += front * velocity;
  if (direction == CameraMovement::Backward)
    position -= front * velocity;
  if (direction == CameraMovement::Left)
    position -= right * velocity;
  if (direction == CameraMovement::Right)
    position += right * velocity;
  if (direction == CameraMovement::Up)
    position += worldUp * velocity;
  if (direction == CameraMovement::Down)
    position -= worldUp * velocity;
}

void Camera::processMouseMovement(float xOffset, float yOffset,
                                  bool constrainPitch) {
  xOffset *= mouseSensitivity;
  yOffset *= mouseSensitivity;

  if (mode == CameraMode::Orbit) {
    orbit(xOffset, yOffset);
    return;
  }

  yaw += xOffset;
  pitch += yOffset;

  // Constrain pitch to avoid gimbal lock
  if (constrainPitch) {
    pitch = glm::clamp(pitch, -89.0f, 89.0f);
  }

  updateCameraVectors();
}

void Camera::processMouseScroll(float yOffset) {
  if (mode == CameraMode::Orbit) {
    orbitDistance -= yOffset * 0.5f;
    orbitDistance = glm::clamp(orbitDistance, 0.5f, 100.0f);
    updateOrbitPosition();
    return;
  }

  zoom -= yOffset;
  zoom = glm::clamp(zoom, 1.0f, 120.0f);
}

void Camera::setOrbitTarget(const glm::vec3 &target) {
  orbitTarget = target;
  updateOrbitPosition();
}

void Camera::orbit(float deltaYaw, float deltaPitch) {
  orbitYaw += deltaYaw;
  orbitPitch += deltaPitch;
  orbitPitch = glm::clamp(orbitPitch, -89.0f, 89.0f);
  updateOrbitPosition();
}

void Camera::setOrbitDistance(float distance) {
  orbitDistance = glm::max(distance, 0.5f);
  updateOrbitPosition();
}

void Camera::setMode(CameraMode newMode) {
  if (mode == newMode)
    return;

  mode = newMode;

  if (mode == CameraMode::Orbit) {
    // Calculate initial orbit parameters from current position
    glm::vec3 dir = position - orbitTarget;
    orbitDistance = glm::length(dir);
    if (orbitDistance < 0.5f)
      orbitDistance = 3.0f;

    dir = glm::normalize(dir);
    orbitPitch = glm::degrees(asin(dir.y));
    orbitYaw = glm::degrees(atan2(dir.x, dir.z));
  }

  updateCameraVectors();
}

void Camera::reset() {
  position = glm::vec3(0.0f, 0.0f, 3.0f);
  yaw = DEFAULT_YAW;
  pitch = DEFAULT_PITCH;
  zoom = DEFAULT_ZOOM;
  movementSpeed = DEFAULT_SPEED;
  mouseSensitivity = DEFAULT_SENSITIVITY;
  nearPlane = DEFAULT_NEAR;
  farPlane = DEFAULT_FAR;
  mode = CameraMode::FreeCamera;
  orbitTarget = glm::vec3(0.0f);
  orbitDistance = 5.0f;
  orbitYaw = 0.0f;
  orbitPitch = 30.0f;
  updateCameraVectors();
}

void Camera::updateCameraVectors() {
  if (mode == CameraMode::Orbit) {
    updateOrbitPosition();
    return;
  }

  // Calculate front vector from Euler angles
  glm::vec3 newFront;
  newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  newFront.y = sin(glm::radians(pitch));
  newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  front = glm::normalize(newFront);

  // Recalculate right and up vectors
  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}

void Camera::updateOrbitPosition() {
  // Calculate position from spherical coordinates
  float x = orbitDistance * cos(glm::radians(orbitPitch)) *
            sin(glm::radians(orbitYaw));
  float y = orbitDistance * sin(glm::radians(orbitPitch));
  float z = orbitDistance * cos(glm::radians(orbitPitch)) *
            cos(glm::radians(orbitYaw));

  position = orbitTarget + glm::vec3(x, y, z);
  front = glm::normalize(orbitTarget - position);
  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}

} // namespace AhnrealEngine
