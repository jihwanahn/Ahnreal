#include "Input.h"

namespace AhnrealEngine {

// Static member definitions
GLFWwindow *Input::window = nullptr;

bool Input::keys[MAX_KEYS] = {false};
bool Input::keysLastFrame[MAX_KEYS] = {false};
bool Input::mouseButtons[MAX_MOUSE_BUTTONS] = {false};
bool Input::mouseButtonsLastFrame[MAX_MOUSE_BUTTONS] = {false};

double Input::mouseX = 0.0;
double Input::mouseY = 0.0;
double Input::lastMouseX = 0.0;
double Input::lastMouseY = 0.0;
double Input::mouseDeltaX = 0.0;
double Input::mouseDeltaY = 0.0;
float Input::scrollDeltaY = 0.0f;
float Input::scrollAccumulator = 0.0f;
bool Input::firstMouse = true;
bool Input::mouseCaptured = false;

std::function<void(float, float)> Input::externalScrollCallback = nullptr;
std::function<void(double, double)> Input::externalMouseMoveCallback = nullptr;

void Input::init(GLFWwindow *win) {
  window = win;

  // Set GLFW callbacks
  glfwSetKeyCallback(window, keyCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwSetScrollCallback(window, scrollCallback);

  // Initialize mouse position
  glfwGetCursorPos(window, &mouseX, &mouseY);
  lastMouseX = mouseX;
  lastMouseY = mouseY;
}

void Input::update() {
  // Store last frame state
  for (int i = 0; i < MAX_KEYS; ++i) {
    keysLastFrame[i] = keys[i];
  }
  for (int i = 0; i < MAX_MOUSE_BUTTONS; ++i) {
    mouseButtonsLastFrame[i] = mouseButtons[i];
  }

  // Calculate mouse delta
  mouseDeltaX = mouseX - lastMouseX;
  mouseDeltaY = lastMouseY - mouseY; // Inverted for typical camera controls
  lastMouseX = mouseX;
  lastMouseY = mouseY;

  // Reset scroll delta (scroll is an event, not continuous state)
  scrollDeltaY = scrollAccumulator;
  scrollAccumulator = 0.0f;
}

bool Input::isKeyPressed(int key) {
  if (key < 0 || key >= MAX_KEYS)
    return false;
  return keys[key];
}

bool Input::isKeyJustPressed(int key) {
  if (key < 0 || key >= MAX_KEYS)
    return false;
  return keys[key] && !keysLastFrame[key];
}

bool Input::isKeyJustReleased(int key) {
  if (key < 0 || key >= MAX_KEYS)
    return false;
  return !keys[key] && keysLastFrame[key];
}

bool Input::isMouseButtonPressed(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return mouseButtons[button];
}

bool Input::isMouseButtonJustPressed(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return mouseButtons[button] && !mouseButtonsLastFrame[button];
}

bool Input::isMouseButtonJustReleased(int button) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return false;
  return !mouseButtons[button] && mouseButtonsLastFrame[button];
}

glm::vec2 Input::getMousePosition() {
  return glm::vec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
}

glm::vec2 Input::getMouseDelta() {
  return glm::vec2(static_cast<float>(mouseDeltaX),
                   static_cast<float>(mouseDeltaY));
}

float Input::getScrollDelta() { return scrollDeltaY; }

void Input::setMouseCaptured(bool captured) {
  mouseCaptured = captured;
  if (window) {
    glfwSetInputMode(window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  }
  if (captured) {
    firstMouse = true;
  }
}

bool Input::isMouseCaptured() { return mouseCaptured; }

void Input::setScrollCallback(std::function<void(float, float)> callback) {
  externalScrollCallback = callback;
}

void Input::setMouseMoveCallback(std::function<void(double, double)> callback) {
  externalMouseMoveCallback = callback;
}

void Input::keyCallback(GLFWwindow *window, int key, int scancode, int action,
                        int mods) {
  if (key < 0 || key >= MAX_KEYS)
    return;

  if (action == GLFW_PRESS) {
    keys[key] = true;
  } else if (action == GLFW_RELEASE) {
    keys[key] = false;
  }
}

void Input::mouseButtonCallback(GLFWwindow *window, int button, int action,
                                int mods) {
  if (button < 0 || button >= MAX_MOUSE_BUTTONS)
    return;

  if (action == GLFW_PRESS) {
    mouseButtons[button] = true;
  } else if (action == GLFW_RELEASE) {
    mouseButtons[button] = false;
  }
}

void Input::cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  if (firstMouse) {
    lastMouseX = xpos;
    lastMouseY = ypos;
    firstMouse = false;
  }

  mouseX = xpos;
  mouseY = ypos;

  if (externalMouseMoveCallback) {
    externalMouseMoveCallback(xpos, ypos);
  }
}

void Input::scrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
  scrollAccumulator += static_cast<float>(yoffset);

  if (externalScrollCallback) {
    externalScrollCallback(static_cast<float>(xoffset),
                           static_cast<float>(yoffset));
  }
}

} // namespace AhnrealEngine
