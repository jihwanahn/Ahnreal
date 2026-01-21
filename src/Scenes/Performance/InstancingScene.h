#pragma once

#include "../../Engine/Scene/Scene.h"
#include "../../Engine/Core/Camera.h"
#include "../../Engine/Renderer/Model.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace AhnrealEngine {

    struct InstanceData {
        glm::mat4 model;
    };

    struct CameraData {
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec4 frustumPlanes[6];
    };

    // Match VkDrawIndexedIndirectCommand logic
    struct VkDrawIndexedIndirectCommand {
        uint32_t    indexCount;
        uint32_t    instanceCount;
        uint32_t    firstIndex;
        int32_t     vertexOffset;
        uint32_t    firstInstance;
    };

    class InstancingScene : public Scene {
    public:
        InstancingScene();
        ~InstancingScene();

        void initialize() override; // Pure virtual implementation
        void initialize(VulkanRenderer* renderer) override;
        void preRender(VulkanRenderer* renderer) override;
        void update(float deltaTime) override;
        void render(VulkanRenderer* renderer) override;
        void onImGuiRender() override;

    private:
        void cleanup();
        void createBuffers();
        void createComputePipeline();
        void createGraphicsPipeline(VulkanRenderer* renderer);
        void createDescriptorSets();
        void updateCameraBuffer();

        VulkanDevice* device = nullptr;
        Camera camera;

        // Resources
        // We will use a simple cube mesh directly or reuse Model class logic
        // For simplicity, let's manually create a cube mesh here or reuse the loaded model
        std::unique_ptr<Model> cubeModel; // We will use this to get vertex/index data

        // Buffers
        static const uint32_t INSTANCE_COUNT = 10000;
        
        VkBuffer instanceBuffer = VK_NULL_HANDLE;
        VkDeviceMemory instanceBufferMemory = VK_NULL_HANDLE;

        VkBuffer cameraBuffer = VK_NULL_HANDLE;
        VkDeviceMemory cameraBufferMemory = VK_NULL_HANDLE;
        void* cameraBufferMapped = nullptr;

        VkBuffer indirectDrawBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indirectDrawBufferMemory = VK_NULL_HANDLE;

        VkBuffer visibleInstanceBuffer = VK_NULL_HANDLE;
        VkDeviceMemory visibleInstanceBufferMemory = VK_NULL_HANDLE;

        // Pipelines
        VkPipeline computePipeline = VK_NULL_HANDLE;
        VkPipelineLayout computePipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout computeDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool computeDescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet computeDescriptorSet = VK_NULL_HANDLE;

        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        VkPipelineLayout graphicsPipelineLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout graphicsSet0Layout = VK_NULL_HANDLE; // Camera
        VkDescriptorSetLayout graphicsDescriptorSetLayout = VK_NULL_HANDLE; // Instance (Set 1)
        VkDescriptorPool graphicsDescriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> graphicsDescriptorSets; // Set 0 (Cam), Set 1 (Inst)

        // Sync
        // We might need a fence if we do async compute, but here we serialize in one command buffer
        
        bool freezeCulling = false;
        int visibleCountCheck = 0; // Readback for debug UI (optional, expensive)
    };
}
