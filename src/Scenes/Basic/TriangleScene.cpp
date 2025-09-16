#include "TriangleScene.h"
#include "../../Engine/Renderer/VulkanDevice.h"
#include "../../Engine/Renderer/VulkanRenderer.h"
#include <imgui.h>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <iostream>
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

namespace AhnrealEngine {

    VkVertexInputBindingDescription Vertex::getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }

    TriangleScene::TriangleScene() : Scene("Triangle Scene") {}

    TriangleScene::~TriangleScene() {
        cleanup();
    }

    void TriangleScene::initialize() {
        // Empty - use initialize(renderer) instead
    }

    void TriangleScene::initialize(VulkanRenderer* renderer) {
        std::cout << "TriangleScene::initialize() called" << std::endl;
        device = renderer->getDevice();
        std::cout << "Device set: " << (device ? "SUCCESS" : "FAILED") << std::endl;

        createVertexBuffer();
        std::cout << "Vertex buffer created" << std::endl;

        createGraphicsPipeline(renderer);
        std::cout << "Graphics pipeline created" << std::endl;
    }

    void TriangleScene::update(float deltaTime) {
        currentRotation += rotationSpeed * deltaTime;
        if (currentRotation > 2.0f * 3.14159f) {
            currentRotation -= 2.0f * 3.14159f;
        }
    }

    void TriangleScene::render(VulkanRenderer* renderer) {
        if (!device) {
            std::cout << "TriangleScene::render: Device is null!" << std::endl;
            return;
        }
        if (graphicsPipeline == VK_NULL_HANDLE) {
            std::cout << "TriangleScene::render: Graphics pipeline is null!" << std::endl;
            return;
        }
        if (vertexBuffer == VK_NULL_HANDLE) {
            std::cout << "TriangleScene::render: Vertex buffer is null!" << std::endl;
            return;
        }

        VkCommandBuffer commandBuffer = renderer->getCurrentCommandBuffer();
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Create rotation matrix
        float cosAngle = std::cos(currentRotation);
        float sinAngle = std::sin(currentRotation);
        glm::mat2 rotationMatrix = glm::mat2(
            cosAngle, -sinAngle,
            sinAngle, cosAngle
        );

        // Update push constants
        PushConstantData pushConstants{};
        pushConstants.transform = rotationMatrix;
        pushConstants.color = triangleColor;
        pushConstants.useBarycentricColors = useBarycentricColors ? 1 : 0;

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pushConstants);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    }

    void TriangleScene::cleanup() {
        std::cout << "TriangleScene::cleanup() called" << std::endl;
        if (device) {
            std::cout << "Device exists, waiting for idle..." << std::endl;
            // Wait for device to be idle before cleanup
            vkDeviceWaitIdle(device->device());
            std::cout << "Device idle complete" << std::endl;

            if (graphicsPipeline != VK_NULL_HANDLE) {
                std::cout << "Destroying graphics pipeline..." << std::endl;
                vkDestroyPipeline(device->device(), graphicsPipeline, nullptr);
                graphicsPipeline = VK_NULL_HANDLE;
            }
            if (pipelineLayout != VK_NULL_HANDLE) {
                std::cout << "Destroying pipeline layout..." << std::endl;
                vkDestroyPipelineLayout(device->device(), pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
            if (vertexBuffer != VK_NULL_HANDLE) {
                std::cout << "Destroying vertex buffer..." << std::endl;
                vkDestroyBuffer(device->device(), vertexBuffer, nullptr);
                vertexBuffer = VK_NULL_HANDLE;
            }
            if (vertexBufferMemory != VK_NULL_HANDLE) {
                std::cout << "Freeing vertex buffer memory..." << std::endl;
                vkFreeMemory(device->device(), vertexBufferMemory, nullptr);
                vertexBufferMemory = VK_NULL_HANDLE;
            }
        }
        std::cout << "TriangleScene::cleanup() completed" << std::endl;
    }

    void TriangleScene::onImGuiRender() {
        ImGui::Begin("Triangle Scene Controls");
        
        ImGui::Text("Basic Triangle Rendering");
        ImGui::Separator();
        
        ImGui::ColorEdit3("Triangle Color", &triangleColor.x);
        ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 5.0f);
        ImGui::Text("Current Rotation: %.2f radians", currentRotation);
        
        ImGui::Separator();
        ImGui::Checkbox("Use Barycentric Colors", &useBarycentricColors);
        if (useBarycentricColors) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Showing barycentric coordinates as colors");
            ImGui::Text("Red = Vertex 0, Green = Vertex 1, Blue = Vertex 2");
        }
        
        if (ImGui::Button("Reset Rotation")) {
            currentRotation = 0.0f;
        }
        
        ImGui::Separator();
        ImGui::Text("Vertices:");
        for (size_t i = 0; i < vertices.size(); ++i) {
            ImGui::Text("Vertex %zu: (%.2f, %.2f) Color: (%.2f, %.2f, %.2f)", 
                i, vertices[i].pos.x, vertices[i].pos.y,
                vertices[i].color.r, vertices[i].color.g, vertices[i].color.b);
        }
        
        ImGui::End();
    }

    void TriangleScene::createVertexBuffer() {
        if (!device) {
            std::cout << "createVertexBuffer: Device is null!" << std::endl;
            return;
        }

        std::cout << "createVertexBuffer: vertices.size() = " << vertices.size() << std::endl;
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        std::cout << "createVertexBuffer: bufferSize = " << bufferSize << std::endl;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device->device(), stagingBufferMemory);

        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        device->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
        vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
    }

    void TriangleScene::createGraphicsPipeline(VulkanRenderer* renderer) {
        if (!device) return;

        // Load shaders
        auto vertShaderCode = readFile("shaders/triangle.vert.spv");
        auto fragShaderCode = readFile("shaders/triangle.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
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
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantData);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

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
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderer->getSwapChainRenderPass();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device->device(), fragShaderModule, nullptr);
        vkDestroyShaderModule(device->device(), vertShaderModule, nullptr);
    }

    VkShaderModule TriangleScene::createShaderModule(const std::vector<char>& code) {
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

    std::vector<char> TriangleScene::readFile(const std::string& filename) {
        std::cout << "Attempting to load shader file: " << filename << std::endl;

        // Print current working directory for debugging
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::cout << "Current working directory: " << cwd << std::endl;
        }

        // Try different paths
        std::vector<std::string> pathsToTry = {
            filename,
            "Debug/" + filename,
            "Release/" + filename,
            "build/Debug/" + filename,
            "../" + filename,
            "./" + filename
        };

        std::ifstream file;
        std::string actualPath;

        for (const auto& path : pathsToTry) {
            file.open(path, std::ios::ate | std::ios::binary);
            if (file.is_open()) {
                actualPath = path;
                std::cout << "Successfully opened shader at: " << actualPath << std::endl;
                break;
            }
            std::cout << "Failed to open: " << path << std::endl;
        }

        if (!file.is_open()) {
            std::cout << "Failed to open shader file in any location: " << filename << std::endl;
            throw std::runtime_error("failed to open file: " + filename);
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        std::cout << "Successfully loaded shader: " << filename << " (size: " << fileSize << " bytes)" << std::endl;
        return buffer;
    }
}