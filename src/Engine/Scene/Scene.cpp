#include "Scene.h"
#include "../Renderer/VulkanRenderer.h"
#include <algorithm>

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
            if (currentScene) {
                currentScene->cleanup();
            }
            currentScene = it->get();
            currentScene->initialize();
        }
    }

    void SceneManager::setCurrentScene(const std::string& name, VulkanRenderer* renderer) {
        auto it = std::find_if(scenes.begin(), scenes.end(),
            [&name](const std::unique_ptr<Scene>& scene) {
                return scene->getName() == name;
            });

        if (it != scenes.end()) {
            if (currentScene) {
                currentScene->cleanup();
            }
            currentScene = it->get();
            currentScene->initialize(renderer);
        }
    }

    void SceneManager::update(float deltaTime) {
        if (currentScene) {
            currentScene->update(deltaTime);
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