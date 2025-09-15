#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <chrono>

namespace AhnrealEngine {
    
    class VulkanDevice;
    class VulkanRenderer;
    class UISystem;
    class SceneManager;

    class Application {
    public:
        Application();
        virtual ~Application();
        
        void run();
        
        GLFWwindow* getWindow() const { return window; }
        
        static constexpr int WIDTH = 1200;
        static constexpr int HEIGHT = 800;
        
    private:
        void initWindow();
        void initVulkan();
        void initUI();
        void mainLoop();
        void cleanup();
        
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
        
        GLFWwindow* window;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        
        std::unique_ptr<VulkanDevice> device;
        std::unique_ptr<VulkanRenderer> renderer;
        std::unique_ptr<UISystem> uiSystem;
        std::unique_ptr<SceneManager> sceneManager;
        
        bool framebufferResized = false;
        
        void createInstance();
        void setupDebugMessenger();
        void createSurface();
        
        bool checkValidationLayerSupport();
        std::vector<const char*> getRequiredExtensions();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);
    };
}