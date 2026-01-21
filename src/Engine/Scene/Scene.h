#pragma once

#include <string>
#include <memory>
#include <vector>

namespace AhnrealEngine {
    
    class VulkanRenderer;

    class Scene {
    public:
        Scene(const std::string& name) : sceneName(name) {}
        virtual ~Scene() = default;
        
        virtual void initialize() = 0;
        virtual void initialize(VulkanRenderer* renderer) { initialize(); }
        virtual void preRender(VulkanRenderer* renderer) {}
        virtual void update(float deltaTime) = 0;
        virtual void render(VulkanRenderer* renderer) = 0;
        virtual void cleanup() = 0;
        virtual void onImGuiRender() = 0;
        
        const std::string& getName() const { return sceneName; }
        
    protected:
        std::string sceneName;
    };

    class SceneManager {
    public:
        SceneManager();
        ~SceneManager();
        
        void addScene(std::unique_ptr<Scene> scene);
        void setCurrentScene(const std::string& name);
        void setCurrentScene(const std::string& name, VulkanRenderer* renderer);
        void update(float deltaTime);
        void preRender(VulkanRenderer* renderer);
        void render(VulkanRenderer* renderer);
        void renderUI();
        void cleanup();
        
        bool processPendingSwitch(VulkanRenderer* renderer);
        
        Scene* getCurrentScene() const { return currentScene; }
        const std::vector<std::unique_ptr<Scene>>& getScenes() const { return scenes; }
        
    private:
        std::vector<std::unique_ptr<Scene>> scenes;
        Scene* currentScene = nullptr;
        Scene* nextScene = nullptr;
    };
}