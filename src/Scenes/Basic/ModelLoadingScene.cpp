#include "ModelLoadingScene.h"
#include "../../Engine/Renderer/VulkanRenderer.h"
#include "../../Engine/Renderer/VulkanDevice.h"
#include "../../Engine/Core/Input.h"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <fstream>
#include <iostream>

namespace AhnrealEngine {

ModelLoadingScene::ModelLoadingScene() 
    : Scene("3D Model Loading"), camera(glm::vec3(0.0f, 2.0f, 5.0f)) {
}

ModelLoadingScene::~ModelLoadingScene() {
    cleanup();
}

void ModelLoadingScene::initialize() { }

void ModelLoadingScene::initialize(VulkanRenderer* renderer) {
    device = renderer->getDevice();

    // 1. Create Model (Load Async or Sync)
    // For now, we load synchronously. User should provide a valid path.
    // We will create a dummy file if it doesn't exist for testing.
    try {
        model = std::make_unique<Model>(device, modelPath);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load model: " << e.what() << std::endl;
        // In a real engine, we might load a fallback model or show an error
    }

    createDescriptorSetLayout();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createGraphicsPipeline(renderer);
}

void ModelLoadingScene::update(float deltaTime) {
    // Camera controls
    if (Input::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        glm::vec2 delta = Input::getMouseDelta();
        camera.processMouseMovement(delta.x, delta.y);
    }
    
    float scroll = Input::getScrollDelta();
    if (scroll != 0.0f) {
        camera.processMouseScroll(scroll);
    }

    if (Input::isKeyPressed(GLFW_KEY_W)) camera.processKeyboard(CameraMovement::Forward, deltaTime);
    if (Input::isKeyPressed(GLFW_KEY_S)) camera.processKeyboard(CameraMovement::Backward, deltaTime);
    if (Input::isKeyPressed(GLFW_KEY_A)) camera.processKeyboard(CameraMovement::Left, deltaTime);
    if (Input::isKeyPressed(GLFW_KEY_D)) camera.processKeyboard(CameraMovement::Right, deltaTime);
    if (Input::isKeyPressed(GLFW_KEY_SPACE)) camera.processKeyboard(CameraMovement::Up, deltaTime);
    if (Input::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) camera.processKeyboard(CameraMovement::Down, deltaTime);

    // Auto rotate model
    if (autoRotate) {
        currentRotation += rotationSpeed * deltaTime;
    }
}

void ModelLoadingScene::render(VulkanRenderer* renderer) {
    if (!model || graphicsPipeline == VK_NULL_HANDLE) return;

    VkCommandBuffer commandBuffer = renderer->getCurrentCommandBuffer();
    uint32_t currentFrame = renderer->getFrameIndex();
    
    // Update UBO
    updateUniformBuffer(currentFrame);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    model->draw(commandBuffer);
}

void ModelLoadingScene::updateUniformBuffer(uint32_t currentFrame) {
    UniformBufferObject ubo{};
    
    // Model matrix
    ubo.model = glm::rotate(glm::mat4(1.0f), currentRotation, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // View/Proj
    ubo.view = camera.getViewMatrix();
    auto extent = device->getSwapChainSupport().capabilities.currentExtent; // Simplistic way, better to get from renderer
    float aspectRatio = (float)extent.width / (float)extent.height;
    ubo.proj = camera.getProjectionMatrix(aspectRatio);

    // Light/View Pos (for basic shading if shader supports it)
    ubo.lightPos = glm::vec3(2.0f, 4.0f, 2.0f);
    ubo.viewPos = camera.getPosition();

    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void ModelLoadingScene::onImGuiRender() {
    ImGui::Begin("Model Loading Scene");
    
    ImGui::Text("Model: %s", modelPath.c_str());
    ImGui::Separator();

    ImGui::Text("Transform:");
    ImGui::Checkbox("Auto Rotate", &autoRotate);
    ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 5.0f);
    ImGui::SliderFloat("Manual Rotation", &currentRotation, 0.0f, glm::two_pi<float>());

    ImGui::Separator();
    ImGui::Text("Camera:");
    glm::vec3 pos = camera.getPosition();
    ImGui::Text("Pos: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);

    if (ImGui::Button("Reload Model")) {
        // Simple reload logic could go here
    }

    ImGui::Separator();
    ImGui::Text("Camera Mode:");
    
    CameraMode currentMode = camera.getMode();
    const char* modeNames[] = { "Free Camera", "Orbit Camera", "First Person", "Third Person" };
    int currentModeIndex = static_cast<int>(currentMode);
    
    if (ImGui::Combo("Mode", &currentModeIndex, modeNames, IM_ARRAYSIZE(modeNames))) {
        camera.setMode(static_cast<CameraMode>(currentModeIndex));
    }

    if (currentMode == CameraMode::Orbit) {
        ImGui::Indent();
        ImGui::Text("Orbit Settings:");
        
        // Orbit Target Control
        // Ideally this matches the model position
        static glm::vec3 target = glm::vec3(0.0f); 
        if (ImGui::DragFloat3("Target", &target.x, 0.1f)) {
            camera.setOrbitTarget(target);
        }

        float distance = glm::length(camera.getPosition() - target);
        // Note: Camera::setOrbitDistance updates position, so we just display strict distance
        ImGui::Text("Distance: %.2f", distance); 
        ImGui::Unindent();
    }

    ImGui::End();
}

void ModelLoadingScene::cleanup() {
    if (device) {
        vkDeviceWaitIdle(device->device());

        if (graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device->device(), graphicsPipeline, nullptr);
            graphicsPipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device->device(), pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }

        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device->device(), descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device->device(), descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < uniformBuffers.size(); i++) {
            vkDestroyBuffer(device->device(), uniformBuffers[i], nullptr);
            vkFreeMemory(device->device(), uniformBuffersMemory[i], nullptr);
        }
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();
    }
    // Model is unique_ptr, handled automatically
}

void ModelLoadingScene::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

// ... Boilerplate for uniform buffers, descriptor pool/sets (copied from Triangle/Cube scene pattern)
void ModelLoadingScene::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(2);
    uniformBuffersMemory.resize(2);
    uniformBuffersMapped.resize(2);

    for (size_t i = 0; i < 2; i++) {
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            uniformBuffers[i], uniformBuffersMemory[i]);
        vkMapMemory(device->device(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void ModelLoadingScene::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 2; // frames in flight

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 2;

    if (vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void ModelLoadingScene::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(2, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(2);
    if (vkAllocateDescriptorSets(device->device(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < 2; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device->device(), 1, &descriptorWrite, 0, nullptr);
    }
}

void ModelLoadingScene::createGraphicsPipeline(VulkanRenderer* renderer) {
    // Reuse cube shaders for now as they support basic 3D projection
    // Ideally we should create a new shader for models (e.g., with normal support)
    auto vertShaderCode = readFile("shaders/cube.vert.spv"); 
    auto fragShaderCode = readFile("shaders/cube.frag.spv"); 

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", nullptr },
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", nullptr }
    };

    // Vertex Input State - USING MESH VERTEX DEFINITION
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Assimp defaults to CCW usually, check imported flags
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(device->device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderer->getSwapChainRenderPass();
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device->device(), fragShaderModule, nullptr);
    vkDestroyShaderModule(device->device(), vertShaderModule, nullptr);
}

VkShaderModule ModelLoadingScene::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device->device(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

std::vector<char> ModelLoadingScene::readFile(const std::string& filename) {
    // Enhanced file read helper that searches multiple paths
    std::vector<std::string> pathsToTry = {
        filename,
        "../" + filename,
        "../../" + filename,
        "build/Debug/" + filename,
        "build/Release/" + filename,
        "shaders/" + filename // Try local shaders dir if filename is just the name
    };

    // Also check relative to executable if possible, but for now just try these common locations
    
    std::ifstream file;
    std::string actualPath;
    
    for (const auto& path : pathsToTry) {
        file.open(path, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            actualPath = path;
            std::cout << "Successfully opened file at: " << actualPath << std::endl;
            break;
        }
    }

    if (!file.is_open()) {
        std::cerr << "Failed to find file: " << filename << " in search paths." << std::endl;
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

} // namespace AhnrealEngine
