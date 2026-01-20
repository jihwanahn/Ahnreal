#pragma once

#include "../../Engine/Core/Camera.h"
#include "../../Engine/Scene/Scene.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>


namespace AhnrealEngine {

class VulkanDevice;

struct CameraTestVertex {
  glm::vec3 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions();
};

struct CameraTestUBO {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

class CameraTestScene : public Scene {
public:
  CameraTestScene();
  ~CameraTestScene() override;

  void initialize() override;
  void initialize(VulkanRenderer *renderer) override;
  void update(float deltaTime) override;
  void render(VulkanRenderer *renderer) override;
  void cleanup() override;
  void onImGuiRender() override;

  void setDevice(VulkanDevice *dev) { device = dev; }

  // Camera access for external control
  Camera &getCamera() { return camera; }

private:
  void createVertexBuffer();
  void createIndexBuffer();
  void createUniformBuffers();
  void createDescriptorSetLayout();
  void createDescriptorPool();
  void createDescriptorSets();
  void updateUniformBuffer(int cubeIndex);
  void createGraphicsPipeline(VulkanRenderer *renderer);

  VulkanDevice *device = nullptr;
  Camera camera;

  // Grid settings
  int gridSize = 5;
  float gridSpacing = 2.0f;

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

  // One UBO per cube (or use dynamic UBO)
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void *> uniformBuffersMapped;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  VkPipeline graphicsPipeline = VK_NULL_HANDLE;
  VkPipeline wireframePipeline = VK_NULL_HANDLE;

  std::vector<CameraTestVertex> vertices = {
      // Front face
      {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}},

      // Back face
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}};

  std::vector<uint16_t> indices = {// Front face
                                   0, 1, 2, 2, 3, 0,
                                   // Back face
                                   4, 6, 5, 6, 4, 7,
                                   // Left face
                                   4, 0, 3, 3, 7, 4,
                                   // Right face
                                   1, 5, 6, 6, 2, 1,
                                   // Bottom face
                                   4, 5, 1, 1, 0, 4,
                                   // Top face
                                   3, 2, 6, 6, 7, 3};

  // UI settings
  float cameraSpeed = 5.0f;
  float mouseSensitivity = 0.1f;
  bool wireframeMode = false;
  bool showGrid = true;

  VkShaderModule createShaderModule(const std::vector<char> &code);
  std::vector<char> readFile(const std::string &filename);
};

} // namespace AhnrealEngine
