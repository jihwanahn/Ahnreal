#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>

namespace AhnrealEngine {

    class VulkanDevice;
    class VulkanSwapChain;

    class VulkanRenderer {
    public:
        VulkanRenderer(GLFWwindow* window, VulkanDevice* device);
        ~VulkanRenderer();

        void recreateSwapChain();
        
        VkCommandBuffer beginFrame();
        void endFrame();
        void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

        VulkanSwapChain* getSwapChain() const;
        VkRenderPass getSwapChainRenderPass() const;
        VkCommandBuffer getCurrentCommandBuffer() const { return commandBuffers[currentFrameIndex]; }
        int getFrameIndex() const { return currentFrameIndex; }
        VkExtent2D getSwapChainExtent() const;

        bool isFrameInProgress() const { return isFrameStarted; }
        VulkanDevice* getDevice() const { return device; }

    private:
        void createCommandBuffers();
        void freeCommandBuffers();
        
        GLFWwindow* window;
        VulkanDevice* device;
        std::unique_ptr<VulkanSwapChain> swapChain;
        std::vector<VkCommandBuffer> commandBuffers;

        uint32_t currentImageIndex;
        int currentFrameIndex = 0;
        bool isFrameStarted = false;
    };
}