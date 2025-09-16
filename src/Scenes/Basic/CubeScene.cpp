#include "CubeScene.h"
#include "../../Engine/Renderer/VulkanDevice.h"
#include "../../Engine/Renderer/VulkanRenderer.h"
#include <imgui.h>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <iostream>
#include <chrono>

namespace AhnrealEngine {

    VkVertexInputBindingDescription Vertex3D::getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex3D);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> Vertex3D::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex3D, color);

        return attributeDescriptions;
    }

    CubeScene::CubeScene() : Scene("Cube Scene") {}

    CubeScene::~CubeScene() {
        cleanup();
    }

    void CubeScene::initialize() {
        // Empty - use initialize(renderer) instead
    }

    void CubeScene::initialize(VulkanRenderer* renderer) {
        device = renderer->getDevice();
        
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSets();
        createGraphicsPipeline(renderer);
    }

    void CubeScene::update(float deltaTime) {
        currentRotation += rotationSpeed * deltaTime;
        if (currentRotation > 2.0f * 3.14159f) {
            currentRotation -= 2.0f * 3.14159f;
        }

        // Check if color settings changed
        if (cubeColor.x != lastCubeColor.x || cubeColor.y != lastCubeColor.y || cubeColor.z != lastCubeColor.z ||
            useBarycentricColors != lastUseBarycentricColors) {
            updateVertexColors();
            lastCubeColor = cubeColor;
            lastUseBarycentricColors = useBarycentricColors;
        }

        updateUniformBuffer();
    }

    void CubeScene::render(VulkanRenderer* renderer) {
        if (!device || graphicsPipeline == VK_NULL_HANDLE) return;

        VkCommandBuffer commandBuffer = renderer->getCurrentCommandBuffer();

        // Choose pipeline based on wireframe mode
        VkPipeline currentPipeline = wireframeMode ? wireframePipeline : graphicsPipeline;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        uint32_t currentFrame = renderer->getFrameIndex();
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }

    void CubeScene::cleanup() {
        if (device) {
            // Wait for device to be idle before cleanup
            vkDeviceWaitIdle(device->device());
            
            // Cleanup uniform buffers - unmap first, then destroy
            for (size_t i = 0; i < uniformBuffers.size(); i++) {
                if (uniformBuffersMapped[i]) {
                    vkUnmapMemory(device->device(), uniformBuffersMemory[i]);
                    uniformBuffersMapped[i] = nullptr;
                }
                if (uniformBuffers[i] != VK_NULL_HANDLE) {
                    vkDestroyBuffer(device->device(), uniformBuffers[i], nullptr);
                    uniformBuffers[i] = VK_NULL_HANDLE;
                }
                if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
                    vkFreeMemory(device->device(), uniformBuffersMemory[i], nullptr);
                    uniformBuffersMemory[i] = VK_NULL_HANDLE;
                }
            }
            uniformBuffers.clear();
            uniformBuffersMemory.clear();
            uniformBuffersMapped.clear();
            
            if (descriptorPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device->device(), descriptorPool, nullptr);
                descriptorPool = VK_NULL_HANDLE;
            }
            
            if (descriptorSetLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(device->device(), descriptorSetLayout, nullptr);
                descriptorSetLayout = VK_NULL_HANDLE;
            }
            
            if (graphicsPipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device->device(), graphicsPipeline, nullptr);
                graphicsPipeline = VK_NULL_HANDLE;
            }

            if (wireframePipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device->device(), wireframePipeline, nullptr);
                wireframePipeline = VK_NULL_HANDLE;
            }
            
            if (pipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device->device(), pipelineLayout, nullptr);
                pipelineLayout = VK_NULL_HANDLE;
            }
            
            if (indexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device->device(), indexBuffer, nullptr);
                indexBuffer = VK_NULL_HANDLE;
            }
            if (indexBufferMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device->device(), indexBufferMemory, nullptr);
                indexBufferMemory = VK_NULL_HANDLE;
            }
            
            if (vertexBuffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(device->device(), vertexBuffer, nullptr);
                vertexBuffer = VK_NULL_HANDLE;
            }
            if (vertexBufferMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device->device(), vertexBufferMemory, nullptr);
                vertexBufferMemory = VK_NULL_HANDLE;
            }
        }
    }

    void CubeScene::onImGuiRender() {
        ImGui::Begin("Cube Scene Controls");

        ImGui::Text("3D Cube Rendering");
        ImGui::Separator();

        // Color settings
        ImGui::Text("Color Settings:");
        ImGui::Checkbox("Use Barycentric Colors", &useBarycentricColors);

        if (!useBarycentricColors) {
            ImGui::ColorEdit3("Cube Color", &cubeColor.x);
        } else {
            ImGui::TextDisabled("Barycentric coloring enabled");
        }

        ImGui::Separator();

        // Rotation settings
        ImGui::Text("Rotation Settings:");
        ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.0f, 5.0f);
        ImGui::SliderFloat3("Rotation Axis", &rotationAxis.x, -1.0f, 1.0f);
        ImGui::Text("Current Rotation: %.2f radians", currentRotation);

        if (ImGui::Button("Reset Rotation")) {
            currentRotation = 0.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Axis")) {
            rotationAxis = glm::vec3(0.0f, 1.0f, 1.0f);
        }

        ImGui::Separator();

        // Rendering settings
        ImGui::Text("Rendering Settings:");
        ImGui::Checkbox("Wireframe Mode", &wireframeMode);

        ImGui::Separator();
        ImGui::Text("Cube Info:");
        ImGui::Text("Vertices: %zu", vertices.size());
        ImGui::Text("Triangles: %zu", indices.size() / 3);

        ImGui::End();
    }

    void CubeScene::createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        
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

    void CubeScene::createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device->device(), stagingBufferMemory);

        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        device->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

        vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
        vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
    }

    void CubeScene::createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        size_t maxFramesInFlight = 2; // Assuming 2 frames in flight
        uniformBuffers.resize(maxFramesInFlight);
        uniformBuffersMemory.resize(maxFramesInFlight);
        uniformBuffersMapped.resize(maxFramesInFlight);

        for (size_t i = 0; i < maxFramesInFlight; i++) {
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device->device(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    void CubeScene::createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void CubeScene::createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(2); // maxFramesInFlight

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(2); // maxFramesInFlight

        if (vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void CubeScene::createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(2, descriptorSetLayout); // maxFramesInFlight
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(2);
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

    void CubeScene::updateUniformBuffer() {
        if (!device) return;
        
        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), currentRotation, glm::normalize(rotationAxis));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1; // Flip Y for Vulkan

        // Update all uniform buffers for safety (in a proper implementation, we'd get current frame)
        for (size_t i = 0; i < uniformBuffersMapped.size(); i++) {
            memcpy(uniformBuffersMapped[i], &ubo, sizeof(ubo));
        }
    }

    void CubeScene::createGraphicsPipeline(VulkanRenderer* renderer) {
        if (!device) return;

        // Load shaders
        auto vertShaderCode = readFile("shaders/cube.vert.spv");
        auto fragShaderCode = readFile("shaders/cube.frag.spv");

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

        auto bindingDescription = Vertex3D::getBindingDescription();
        auto attributeDescriptions = Vertex3D::getAttributeDescriptions();

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

        // Create solid pipeline
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

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

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

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
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create solid graphics pipeline!");
        }

        // Create wireframe pipeline
        VkPipelineRasterizationStateCreateInfo wireframeRasterizer = rasterizer;
        wireframeRasterizer.polygonMode = VK_POLYGON_MODE_LINE;

        pipelineInfo.pRasterizationState = &wireframeRasterizer;

        if (vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &wireframePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create wireframe graphics pipeline!");
        }

        vkDestroyShaderModule(device->device(), fragShaderModule, nullptr);
        vkDestroyShaderModule(device->device(), vertShaderModule, nullptr);
    }

    VkShaderModule CubeScene::createShaderModule(const std::vector<char>& code) {
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

    std::vector<char> CubeScene::readFile(const std::string& filename) {
        std::cout << "CubeScene: Attempting to load shader file: " << filename << std::endl;

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
                std::cout << "CubeScene: Successfully opened shader at: " << actualPath << std::endl;
                break;
            }
        }

        if (!file.is_open()) {
            std::cout << "CubeScene: Failed to open shader file in any location: " << filename << std::endl;
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

    void CubeScene::updateVertexColors() {
        if (!device) return;

        if (useBarycentricColors) {
            // Use barycentric coordinate-based coloring for each face
            // Each vertex of each triangle face gets red, green, or blue based on its position

            // Front face (red-green-blue pattern)
            vertices[0].color = {1.0f, 0.0f, 0.0f}; // Red
            vertices[1].color = {0.0f, 1.0f, 0.0f}; // Green
            vertices[2].color = {0.0f, 0.0f, 1.0f}; // Blue
            vertices[3].color = {1.0f, 1.0f, 0.0f}; // Yellow

            // Back face (magenta-cyan-white pattern)
            vertices[4].color = {1.0f, 0.0f, 1.0f}; // Magenta
            vertices[5].color = {0.0f, 1.0f, 1.0f}; // Cyan
            vertices[6].color = {1.0f, 1.0f, 1.0f}; // White
            vertices[7].color = {0.5f, 0.5f, 0.5f}; // Gray
        } else {
            // Use uniform color
            for (auto& vertex : vertices) {
                vertex.color = cubeColor;
            }
        }

        // Update vertex buffer with new colors
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device->device(), stagingBufferMemory);

        device->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
        vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
    }
}