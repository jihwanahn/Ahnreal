#pragma once

#include "../../Engine/Scene/Scene.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <string>

namespace AhnrealEngine {

    class VulkanDevice;

    struct Vertex3D {
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    class CubeScene : public Scene {
    public:
        CubeScene();
        ~CubeScene() override;

        void initialize() override;
        void initialize(VulkanRenderer* renderer) override;
        void update(float deltaTime) override;
        void render(VulkanRenderer* renderer) override;
        void cleanup() override;
        void onImGuiRender() override;

        void setDevice(VulkanDevice* dev) { device = dev; }

    private:
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSets();
        void updateUniformBuffer();
        void createGraphicsPipeline(VulkanRenderer* renderer);
        void updateVertexColors();

        VulkanDevice* device = nullptr;
        
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        
        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;
        
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;
        
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        VkPipeline wireframePipeline = VK_NULL_HANDLE;
        
        std::vector<Vertex3D> vertices = {
            // Front face
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
            
            // Back face
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}}
        };
        
        std::vector<uint16_t> indices = {
            // Front face
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
            3, 2, 6, 6, 7, 3
        };
        
        float rotationSpeed = 1.0f;
        float currentRotation = 0.0f;
        glm::vec3 cubeColor = {1.0f, 1.0f, 1.0f};
        glm::vec3 lastCubeColor = {1.0f, 1.0f, 1.0f};
        glm::vec3 rotationAxis = {0.0f, 1.0f, 1.0f};
        bool wireframeMode = false;
        bool colorChanged = false;
        bool useBarycentricColors = false;
        bool lastUseBarycentricColors = false;
        
        VkShaderModule createShaderModule(const std::vector<char>& code);
        std::vector<char> readFile(const std::string& filename);
    };
}