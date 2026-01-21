#include "Scene.h"
#include "../Renderer/VulkanRenderer.h"
#include "../Renderer/VulkanDevice.h"
#include <algorithm>
#include <vector>

namespace AhnrealEngine {

    SceneManager::SceneManager() {}

    SceneManager::~SceneManager() {
        cleanup();
    }

    void SceneManager::addScene(std::unique_ptr<Scene> scene) {
        scenes.push_back(std::move(scene));
    }

    void SceneManager::setCurrentScene(const std::string& name) {
        auto it = std::find_if(scenes.begin(), scenes.end(),
            [&name](const std::unique_ptr<Scene>& scene) {
                return scene->getName() == name;
            });

        if (it != scenes.end()) {
             // Defer switch? No renderer passed here, might be problematic.
             // Assume this one is immediate/unsafe or used during initialization.
             // But for safety, let's just warn or handle it if no renderer.
             // Actually, the hazardous one is the overload with renderer used in runtime.
             nextScene = it->get();
        }
    }

    void SceneManager::setCurrentScene(const std::string& name, VulkanRenderer* renderer) {
         auto it = std::find_if(scenes.begin(), scenes.end(),
            [&name](const std::unique_ptr<Scene>& scene) {
                return scene->getName() == name;
            });

        if (it != scenes.end()) {
            nextScene = it->get();
            // Don't switch immediately. Wait for processPendingSwitch.
        }
    }

    bool SceneManager::processPendingSwitch(VulkanRenderer* renderer) {
        if (nextScene && nextScene != currentScene) {
            if (currentScene) {
                vkDeviceWaitIdle(renderer->getDevice()->device());
                currentScene->cleanup();
            }
            currentScene = nextScene;
            currentScene->initialize(renderer);
            nextScene = nullptr;
            return true;
        }
        return false;
    }

    void SceneManager::update(float deltaTime) {
        if (currentScene) {
            currentScene->update(deltaTime);
        }
    }

    void SceneManager::preRender(VulkanRenderer* renderer) {
        if (currentScene) {
            currentScene->preRender(renderer);
        }
    }

    void SceneManager::render(VulkanRenderer* renderer) {
        if (currentScene) {
            currentScene->render(renderer);
        }
    }

    void SceneManager::renderUI() {
        if (currentScene) {
            currentScene->onImGuiRender();
        }
    }

    void SceneManager::cleanup() {
        if (currentScene) {
            currentScene->cleanup();
            currentScene = nullptr;
        }
        scenes.clear();
    }
}