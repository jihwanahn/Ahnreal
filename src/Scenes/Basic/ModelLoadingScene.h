#pragma once

#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Core/Camera.h"
#include "../../Engine/Renderer/Model.h"
#include <memory>
#include <vulkan/vulkan.h>

namespace AhnrealEngine {

class ModelLoadingScene : public Scene {
public:
    ModelLoadingScene();
    ~ModelLoadingScene() override;

    void initialize() override;
    void initialize(VulkanRenderer* renderer) override;
    void update(float deltaTime) override;
    void render(VulkanRenderer* renderer) override;
    void cleanup() override;
    void onImGuiRender() override;

private:
    void createGraphicsPipeline(VulkanRenderer* renderer);
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createUniformBuffers();
    void createDescriptorSets();
    void updateUniformBuffer(uint32_t currentFrame);

    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);

    VulkanDevice* device = nullptr;
    Camera camera;
    std::unique_ptr<Model> model;
    
    // Resource paths
    std::string modelPath = "models/cube.obj"; // Default test model

    // Vulkan resources
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 lightPos;
        float _padding;
        glm::vec3 viewPos;
        float _padding2;
    };

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    
    // UI Settings
    bool autoRotate = false;
    float rotationSpeed = 1.0f;
    float currentRotation = 0.0f;
};

} // namespace AhnrealEngine
