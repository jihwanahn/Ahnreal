#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <functional>

namespace AhnrealEngine {

    class VulkanDevice;
    class VulkanRenderer;
    class SceneManager;

    class UISystem {
    public:
        UISystem(GLFWwindow* window, VulkanDevice* device, VulkanRenderer* renderer);
        ~UISystem();

        void initialize();
        void newFrame();
        void render();
        void cleanup();

        void setSceneManager(SceneManager* sceneManager) { this->sceneManager = sceneManager; }

    private:
        void renderMainMenuBar();
        void renderSceneSelector();
        void renderSceneControls();
        
        GLFWwindow* window;
        VulkanDevice* device;
        VulkanRenderer* renderer;
        SceneManager* sceneManager = nullptr;
        
        VkDescriptorPool imguiPool;
        bool showSceneSelector = true;
        
        void createDescriptorPool();
    };
}