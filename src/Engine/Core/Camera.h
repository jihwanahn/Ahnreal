#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace AhnrealEngine {

    // Camera movement directions
    enum class CameraMovement {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    // Camera modes
    enum class CameraMode {
        FreeCamera,   // WASD + mouse free look
        Orbit,        // Orbit around a target point
        FirstPerson,  // First person view
        ThirdPerson   // Third person view (future)
    };

    // Default camera values
    constexpr float DEFAULT_YAW = -90.0f;
    constexpr float DEFAULT_PITCH = 0.0f;
    constexpr float DEFAULT_SPEED = 2.5f;
    constexpr float DEFAULT_SENSITIVITY = 0.1f;
    constexpr float DEFAULT_ZOOM = 45.0f;
    constexpr float DEFAULT_NEAR = 0.1f;
    constexpr float DEFAULT_FAR = 100.0f;

    class Camera {
    public:
        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
               glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
               float yaw = DEFAULT_YAW,
               float pitch = DEFAULT_PITCH);
        
        ~Camera() = default;

        // Get view and projection matrices
        glm::mat4 getViewMatrix() const;
        glm::mat4 getProjectionMatrix(float aspectRatio) const;

        // Process input
        void processKeyboard(CameraMovement direction, float deltaTime);
        void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
        void processMouseScroll(float yOffset);

        // Orbit camera controls
        void setOrbitTarget(const glm::vec3& target);
        void orbit(float deltaYaw, float deltaPitch);
        void setOrbitDistance(float distance);

        // Getters
        glm::vec3 getPosition() const { return position; }
        glm::vec3 getFront() const { return front; }
        glm::vec3 getUp() const { return up; }
        glm::vec3 getRight() const { return right; }
        float getYaw() const { return yaw; }
        float getPitch() const { return pitch; }
        float getZoom() const { return zoom; }
        float getNear() const { return nearPlane; }
        float getFar() const { return farPlane; }
        CameraMode getMode() const { return mode; }

        // Setters
        void setPosition(const glm::vec3& pos) { position = pos; updateCameraVectors(); }
        void setYaw(float newYaw) { yaw = newYaw; updateCameraVectors(); }
        void setPitch(float newPitch) { pitch = newPitch; updateCameraVectors(); }
        void setZoom(float newZoom) { zoom = glm::clamp(newZoom, 1.0f, 120.0f); }
        void setNear(float near) { nearPlane = near; }
        void setFar(float far) { farPlane = far; }
        void setMovementSpeed(float speed) { movementSpeed = speed; }
        void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
        void setMode(CameraMode newMode);

        // Reset to default
        void reset();

    private:
        void updateCameraVectors();
        void updateOrbitPosition();

        // Camera attributes
        glm::vec3 position;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::vec3 worldUp;

        // Euler angles
        float yaw;
        float pitch;

        // Camera options
        float movementSpeed;
        float mouseSensitivity;
        float zoom;
        float nearPlane;
        float farPlane;

        // Camera mode
        CameraMode mode = CameraMode::FreeCamera;

        // Orbit mode specific
        glm::vec3 orbitTarget = glm::vec3(0.0f);
        float orbitDistance = 5.0f;
        float orbitYaw = 0.0f;
        float orbitPitch = 30.0f;
    };

} // namespace AhnrealEngine
