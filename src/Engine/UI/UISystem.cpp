#include "UISystem.h"
#include "../Renderer/VulkanDevice.h"
#include "../Renderer/VulkanRenderer.h"
#include "../Renderer/VulkanSwapChain.h"
#include "../Scene/Scene.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

namespace AhnrealEngine {

    UISystem::UISystem(GLFWwindow* window, VulkanDevice* device, VulkanRenderer* renderer) 
        : window{window}, device{device}, renderer{renderer} {
    }

    UISystem::~UISystem() {
        cleanup();
    }

    void UISystem::initialize() {
        createDescriptorPool();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window, true);
        
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = device->instance;
        init_info.PhysicalDevice = device->physicalDevice();
        init_info.Device = device->device();
        init_info.QueueFamily = device->findPhysicalQueueFamilies().graphicsFamily.value();
        init_info.Queue = device->graphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imguiPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = renderer->getSwapChain()->imageCount();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = VK_NULL_HANDLE;
        init_info.CheckVkResultFn = nullptr;
        init_info.RenderPass = renderer->getSwapChainRenderPass();

        ImGui_ImplVulkan_Init(&init_info);

        auto commandBuffer = device->beginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture();
        device->endSingleTimeCommands(commandBuffer);
    }

    void UISystem::newFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMainMenuBar();
        if (showSceneSelector) {
            renderSceneSelector();
        }
        renderSceneControls();
    }

    void UISystem::render() {
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, renderer->getCurrentCommandBuffer());
    }

    void UISystem::cleanup() {
        if (imguiPool != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            vkDestroyDescriptorPool(device->device(), imguiPool, nullptr);
            imguiPool = VK_NULL_HANDLE;
        }
    }

    void UISystem::createDescriptorPool() {
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        if (vkCreateDescriptorPool(device->device(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool for ImGui!");
        }
    }

    void UISystem::renderMainMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Scene Selector", nullptr, &showSceneSelector);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Scenes")) {
                if (sceneManager) {
                    const auto& scenes = sceneManager->getScenes();
                    for (const auto& scene : scenes) {
                        if (ImGui::MenuItem(scene->getName().c_str())) {
                            sceneManager->setCurrentScene(scene->getName(), renderer);
                        }
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    // Show about dialog
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void UISystem::renderSceneSelector() {
        ImGui::Begin("Scene Selector", &showSceneSelector);
        
        ImGui::Text("Available Scenes:");
        ImGui::Separator();
        
        if (sceneManager) {
            const auto& scenes = sceneManager->getScenes();
            Scene* currentScene = sceneManager->getCurrentScene();
            
            for (const auto& scene : scenes) {
                bool isSelected = (currentScene && currentScene->getName() == scene->getName());
                
                if (ImGui::Selectable(scene->getName().c_str(), isSelected)) {
                    sceneManager->setCurrentScene(scene->getName(), renderer);
                }
                
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        
        ImGui::End();
    }

    void UISystem::renderSceneControls() {
        if (sceneManager && sceneManager->getCurrentScene()) {
            // Scene-specific controls are rendered by the scene itself
            // through onImGuiRender()
        }
        
        // Global engine stats
        ImGui::Begin("Engine Stats");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        
        if (sceneManager && sceneManager->getCurrentScene()) {
            ImGui::Text("Current Scene: %s", sceneManager->getCurrentScene()->getName().c_str());
        }
        
        ImGui::End();
    }
}