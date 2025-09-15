#pragma once

#include "../../Engine/Scene/Scene.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace AhnrealEngine {

    class VulkanDevice;

    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    class TriangleScene : public Scene {
    public:
        TriangleScene();
        ~TriangleScene() override;

        void initialize() override;
        void initialize(VulkanRenderer* renderer) override;
        void update(float deltaTime) override;
        void render(VulkanRenderer* renderer) override;
        void cleanup() override;
        void onImGuiRender() override;

        void setDevice(VulkanDevice* dev) { device = dev; }

    private:
        void createVertexBuffer();
        void createGraphicsPipeline(VulkanRenderer* renderer);

        VulkanDevice* device = nullptr;
        
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        
        std::vector<Vertex> vertices = {
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
        
        glm::vec3 triangleColor = {1.0f, 1.0f, 1.0f};
        float rotationSpeed = 1.0f;
        float currentRotation = 0.0f;
        
        VkShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readFile(const std::string& filename);
    };
}