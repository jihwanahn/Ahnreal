#pragma once

#include "VulkanDevice.h"
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace AhnrealEngine {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
  glm::vec3 tangent;
  glm::vec3 bitangent;

  static VkVertexInputBindingDescription getBindingDescription();
  static std::vector<VkVertexInputAttributeDescription>
  getAttributeDescriptions();
};

class Mesh {
public:
  Mesh(VulkanDevice *device, const std::vector<Vertex> &vertices,
       const std::vector<uint32_t> &indices);
  ~Mesh();

  // Disable copying to prevent double-free of Vulkan resources
  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  // Enable moving for efficient vector storage
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  void draw(VkCommandBuffer commandBuffer);

private:
  void createVertexBuffer(const std::vector<Vertex> &vertices);
  void createIndexBuffer(const std::vector<uint32_t> &indices);

  VulkanDevice *device;

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
  uint32_t vertexCount = 0;

  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
  uint32_t indexCount = 0;
};

} // namespace AhnrealEngine
