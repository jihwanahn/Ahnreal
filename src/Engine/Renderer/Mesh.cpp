#include "Mesh.h"
#include <cstring>
#include <stdexcept>

namespace AhnrealEngine {

VkVertexInputBindingDescription Vertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription>
Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

  // Position
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, position);

  // Normal
  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, normal);

  // TexCoord
  attributeDescriptions[2].binding = 0;
  attributeDescriptions[2].location = 2;
  attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
  
  // Tangent
  attributeDescriptions[3].binding = 0;
  attributeDescriptions[3].location = 3;
  attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[3].offset = offsetof(Vertex, tangent);

  // Bitangent
  attributeDescriptions[4].binding = 0;
  attributeDescriptions[4].location = 4;
  attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[4].offset = offsetof(Vertex, bitangent);

  return attributeDescriptions;
}

Mesh::Mesh(VulkanDevice *device, const std::vector<Vertex> &vertices,
           const std::vector<uint32_t> &indices)
    : device(device) {
  createVertexBuffer(vertices);
  createIndexBuffer(indices);
}

Mesh::~Mesh() {
  if (indexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device->device(), indexBuffer, nullptr);
    vkFreeMemory(device->device(), indexBufferMemory, nullptr);
  }

  if (vertexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device->device(), vertexBuffer, nullptr);
    vkFreeMemory(device->device(), vertexBufferMemory, nullptr);
  }
}

Mesh::Mesh(Mesh &&other) noexcept
    : device(other.device), vertexBuffer(other.vertexBuffer),
      vertexBufferMemory(other.vertexBufferMemory),
      vertexCount(other.vertexCount), indexBuffer(other.indexBuffer),
      indexBufferMemory(other.indexBufferMemory), indexCount(other.indexCount) {
  other.vertexBuffer = VK_NULL_HANDLE;
  other.vertexBufferMemory = VK_NULL_HANDLE;
  other.indexBuffer = VK_NULL_HANDLE;
  other.indexBufferMemory = VK_NULL_HANDLE;
  other.vertexCount = 0;
  other.indexCount = 0;
}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
  if (this != &other) {
    // Clean up existing resources
    this->~Mesh();

    // Move resources
    device = other.device;
    vertexBuffer = other.vertexBuffer;
    vertexBufferMemory = other.vertexBufferMemory;
    vertexCount = other.vertexCount;
    indexBuffer = other.indexBuffer;
    indexBufferMemory = other.indexBufferMemory;
    indexCount = other.indexCount;

    // Invalidate other
    other.vertexBuffer = VK_NULL_HANDLE;
    other.vertexBufferMemory = VK_NULL_HANDLE;
    other.indexBuffer = VK_NULL_HANDLE;
    other.indexBufferMemory = VK_NULL_HANDLE;
    other.vertexCount = 0;
    other.indexCount = 0;
  }
  return *this;
}

void Mesh::draw(VkCommandBuffer commandBuffer) {
  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

  if (indexCount > 0) {
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
  } else {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
  }
}

void Mesh::createVertexBuffer(const std::vector<Vertex> &vertices) {
  vertexCount = static_cast<uint32_t>(vertices.size());
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(device->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(device->device(), stagingBufferMemory);

  device->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  device->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

  vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
  vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(const std::vector<uint32_t> &indices) {
  indexCount = static_cast<uint32_t>(indices.size());
  if (indexCount == 0) return;

  VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(device->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device->device(), stagingBufferMemory);

  device->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

  device->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

  vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
  vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
}

} // namespace AhnrealEngine
