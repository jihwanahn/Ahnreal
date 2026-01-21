#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <glm/glm.hpp>


namespace AhnrealEngine {

class Input {
public:
  // Initialize input system with GLFW window
  static void init(GLFWwindow *window);

  // Update input state (call once per frame)
  static void update();

  // Keyboard
  static bool isKeyPressed(int key);
  static bool isKeyJustPressed(int key);
  static bool isKeyJustReleased(int key);

  // Mouse
  static bool isMouseButtonPressed(int button);
  static bool isMouseButtonJustPressed(int button);
  static bool isMouseButtonJustReleased(int button);

  static glm::vec2 getMousePosition();
  static glm::vec2 getMouseDelta();
  static float getScrollDelta();

  // Mouse capture (for FPS-style camera control)
  static void setMouseCaptured(bool captured);
  static bool isMouseCaptured();

  // Callbacks for external handling
  static void setScrollCallback(std::function<void(float, float)> callback);
  static void
  setMouseMoveCallback(std::function<void(double, double)> callback);

private:
  static void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                          int mods);
  static void mouseButtonCallback(GLFWwindow *window, int button, int action,
                                  int mods);
  static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
  static void scrollCallback(GLFWwindow *window, double xoffset,
                             double yoffset);

  static GLFWwindow *window;

  static constexpr int MAX_KEYS = 512;
  static constexpr int MAX_MOUSE_BUTTONS = 8;

  static bool keys[MAX_KEYS];
  static bool keysLastFrame[MAX_KEYS];
  static bool mouseButtons[MAX_MOUSE_BUTTONS];
  static bool mouseButtonsLastFrame[MAX_MOUSE_BUTTONS];

  static double mouseX, mouseY;
  static double lastMouseX, lastMouseY;
  static double mouseDeltaX, mouseDeltaY;
  static float scrollDeltaY;
  static float scrollAccumulator;
  static bool firstMouse;
  static bool mouseCaptured;

  static std::function<void(float, float)> externalScrollCallback;
  static std::function<void(double, double)> externalMouseMoveCallback;
};

} // namespace AhnrealEngine
