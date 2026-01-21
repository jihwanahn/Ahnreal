#include "InstancingScene.h"
#include "../../Engine/Renderer/VulkanRenderer.h"
#include "../../Engine/Renderer/VulkanDevice.h"
#include "../../Engine/Core/Input.h"
#include <imgui.h>
#include <random>
#include <array>
#include <iostream>
#include <fstream>
#include <ctime>

namespace AhnrealEngine {

    InstancingScene::InstancingScene() 
        : Scene("GPU Instancing Culling"), camera(glm::vec3(0.0f, 10.0f, 30.0f)) {
        camera.setFar(200.0f);
    }

    InstancingScene::~InstancingScene() {
        cleanup();
    }

    void InstancingScene::initialize() {
        // Required by base class but we use initialize(renderer)
    }

    void InstancingScene::initialize(VulkanRenderer* renderer) {
        device = renderer->getDevice();
        
        // Load a simple cube model to use its mesh data
        try {
            cubeModel = std::make_unique<Model>(device, "models/cube.obj");
        } catch (...) {
             std::cerr << "Failed to load cube model for instancing, make sure models/cube.obj exists" << std::endl;
        }

        createBuffers();
        createComputePipeline();
        createGraphicsPipeline(renderer); // This now handles descriptor sets internally correctly
        createDescriptorSets(); // This is for Compute
    }

    void InstancingScene::update(float deltaTime) {
        // Camera Controls
        if (Input::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            glm::vec2 delta = Input::getMouseDelta();
            camera.processMouseMovement(delta.x, delta.y);
        }
        float scroll = Input::getScrollDelta();
        if (scroll != 0.0f) camera.processMouseScroll(scroll);

        if (Input::isKeyPressed(GLFW_KEY_W)) camera.processKeyboard(CameraMovement::Forward, deltaTime);
        if (Input::isKeyPressed(GLFW_KEY_S)) camera.processKeyboard(CameraMovement::Backward, deltaTime);
        if (Input::isKeyPressed(GLFW_KEY_A)) camera.processKeyboard(CameraMovement::Left, deltaTime);
        if (Input::isKeyPressed(GLFW_KEY_D)) camera.processKeyboard(CameraMovement::Right, deltaTime);
        
        updateCameraBuffer();
    }

    void InstancingScene::preRender(VulkanRenderer* renderer) {
        VkCommandBuffer commandBuffer = renderer->getCurrentCommandBuffer();
        
        // 1. Reset Atomic Counter via UpdateBuffer
        vkCmdUpdateBuffer(commandBuffer, indirectDrawBuffer, 4, 4, &visibleCountCheck); 
        
        // Barrier: Transfer -> Compute
        VkMemoryBarrier bufferBarrier{};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &bufferBarrier, 0, nullptr, 0, nullptr);

        // 2. Compute Culling
        if (!freezeCulling) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet, 0, nullptr);
            
            // Push Constants
            uint32_t totalInstances = INSTANCE_COUNT;
            vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &totalInstances);

            // Group Count: (10000 + 255) / 256 = 40 groups. 40 * 256 = 10240 threads. Enough for 10000 instances.
            uint32_t groupCount = (INSTANCE_COUNT + 255) / 256;
            vkCmdDispatch(commandBuffer, groupCount, 1, 1);

            // Barrier: Compute -> Draw/Vertex
            VkBufferMemoryBarrier barriers[2] = {};
            // Indirect Argument Barrier
            barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barriers[0].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barriers[0].buffer = indirectDrawBuffer;
            barriers[0].offset = 0;
            barriers[0].size = VK_WHOLE_SIZE;

            // Visible Indices Barrier (Vertex Shader Read)
            barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barriers[1].buffer = visibleInstanceBuffer;
            barriers[1].offset = 0;
            barriers[1].size = VK_WHOLE_SIZE;

            vkCmdPipelineBarrier(commandBuffer, 
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 
                0, 0, nullptr, 2, barriers, 0, nullptr);
        }
    }

    void InstancingScene::render(VulkanRenderer* renderer) {
        VkCommandBuffer commandBuffer = renderer->getCurrentCommandBuffer();

        // 3. Draw
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        // Bind Sets: Set 0 (Camera), Set 1 (Instances/Visible)
        if (graphicsDescriptorSets.size() >= 2) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 2, graphicsDescriptorSets.data(), 0, nullptr);
        }

        if (cubeModel && !cubeModel->getMeshes().empty()) {
            Mesh* mesh = cubeModel->getMeshes()[0].get(); // Just draw first mesh
            VkBuffer vertexBuffers[] = { mesh->getVertexBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexedIndirect(commandBuffer, indirectDrawBuffer, 0, 1, sizeof(VkDrawIndexedIndirectCommand));
        }
    }

    void InstancingScene::createBuffers() {
        // 1. Instance Data Generation
        std::vector<InstanceData> instances(INSTANCE_COUNT);
        std::default_random_engine rnd((unsigned)time(nullptr));
        std::uniform_real_distribution<float> distPos(-50.0f, 50.0f);
        std::uniform_real_distribution<float> distScale(0.5f, 1.5f);
        std::uniform_real_distribution<float> distRot(0.0f, 360.0f);

        for (int i = 0; i < INSTANCE_COUNT; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(distPos(rnd), distPos(rnd), distPos(rnd)));
            model = glm::rotate(model, glm::radians(distRot(rnd)), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(distScale(rnd)));
            instances[i].model = model;
        }

        VkDeviceSize instanceBufferSize = sizeof(InstanceData) * INSTANCE_COUNT;
        
        // Staging
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device->createBuffer(instanceBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device->device(), stagingBufferMemory, 0, instanceBufferSize, 0, &data);
        memcpy(data, instances.data(), (size_t)instanceBufferSize);
        vkUnmapMemory(device->device(), stagingBufferMemory);

        // Instance Buffer (Storage + TransferDst)
        device->createBuffer(instanceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, instanceBuffer, instanceBufferMemory);
        device->copyBuffer(stagingBuffer, instanceBuffer, instanceBufferSize);
        vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
        vkFreeMemory(device->device(), stagingBufferMemory, nullptr);

        // Camera Buffer
        device->createBuffer(sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            cameraBuffer, cameraBufferMemory);
        vkMapMemory(device->device(), cameraBufferMemory, 0, sizeof(CameraData), 0, &cameraBufferMapped);

        // Indirect Draw Buffer
        device->createBuffer(sizeof(VkDrawIndexedIndirectCommand), 
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indirectDrawBuffer, indirectDrawBufferMemory);
        
        // Init Indirect Command
        if (cubeModel && !cubeModel->getMeshes().empty()) {
            VkDrawIndexedIndirectCommand cmd{};
            cmd.indexCount = cubeModel->getMeshes()[0]->getIndexCount();
            cmd.instanceCount = 0; // Start 0
            cmd.firstIndex = 0;
            cmd.vertexOffset = 0;
            cmd.firstInstance = 0;

            device->createBuffer(sizeof(VkDrawIndexedIndirectCommand), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                stagingBuffer, stagingBufferMemory);
            
            vkMapMemory(device->device(), stagingBufferMemory, 0, sizeof(VkDrawIndexedIndirectCommand), 0, &data);
            memcpy(data, &cmd, sizeof(VkDrawIndexedIndirectCommand));
            vkUnmapMemory(device->device(), stagingBufferMemory);
            
            device->copyBuffer(stagingBuffer, indirectDrawBuffer, sizeof(VkDrawIndexedIndirectCommand));
            vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
            vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
        }

        // Visible Instances Buffer
        device->createBuffer(sizeof(uint32_t) * INSTANCE_COUNT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, visibleInstanceBuffer, visibleInstanceBufferMemory);
    }

    void InstancingScene::updateCameraBuffer() {
        CameraData camData{};
        camData.view = camera.getViewMatrix();
        VkExtent2D extent = device->getSwapChainSupport().capabilities.currentExtent;
        float aspect = (float)extent.width / (float)extent.height;
        camData.proj = camera.getProjectionMatrix(aspect);

        // Frustum Culling Planes
        // GLM is column-major, so m[col][row].
        // We want rows of VP matrix.
        // Direct access: Row i = vec4(vp[0][i], vp[1][i], vp[2][i], vp[3][i])
        // Or simple transpose:
        glm::mat4 vp = camData.proj * camData.view;
        glm::mat4 vpT = glm::transpose(vp);
        
        camData.frustumPlanes[0] = vpT[3] + vpT[0]; // Left
        camData.frustumPlanes[1] = vpT[3] - vpT[0]; // Right
        camData.frustumPlanes[2] = vpT[3] + vpT[1]; // Bottom
        camData.frustumPlanes[3] = vpT[3] - vpT[1]; // Top
        camData.frustumPlanes[4] = vpT[3] + vpT[2]; // Near
        camData.frustumPlanes[5] = vpT[3] - vpT[2]; // Far

        for (int i = 0; i < 6; i++) {
            camData.frustumPlanes[i] /= glm::length(glm::vec3(camData.frustumPlanes[i]));
        }

        memcpy(cameraBufferMapped, &camData, sizeof(CameraData));
    }

    void InstancingScene::createComputePipeline() {
        // [0: Instance(S), 1: Cam(U), 2: Indir(S), 3: Vis(S)]
        std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
        
        bindings[0] = {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
        bindings[1] = {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
        bindings[2] = {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
        bindings[3] = {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

        VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr, &computeDescriptorSetLayout);

        VkPushConstantRange pushConstant{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t)};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

        vkCreatePipelineLayout(device->device(), &pipelineLayoutInfo, nullptr, &computePipelineLayout);

        // Load Shader
        auto readFile = [](const std::string& filename) {
             std::vector<std::string> paths = {
                 "build/Debug/" + filename,
                 "../shaders/" + filename,
                 "../../shaders/" + filename, // In case of deeper nesting or different cwd
                 "shaders/" + filename,
                 filename
             };
             
             for (const auto& path : paths) {
                 std::ifstream file(path, std::ios::ate | std::ios::binary);
                 if (file.is_open()) {
                     size_t fileSize = (size_t)file.tellg();
                     std::vector<char> buffer(fileSize);
                     file.seekg(0);
                     file.read(buffer.data(), fileSize);
                     // std::cout << "Loaded shader: " << path << std::endl; // Debug print if needed
                     return buffer;
                 }
             }
             
             throw std::runtime_error("Failed to find/open shader file: " + filename);
        };
        auto computeCode = readFile("cull.comp.spv");
        VkShaderModule computeModule;
        VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = computeCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(computeCode.data());
        vkCreateShaderModule(device->device(), &createInfo, nullptr, &computeModule);
        
        // --- Allocation ---
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 }, // Inst, Indir, Vis
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }  // Cam
        };
        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, 0, 1, 2, poolSizes};
        vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &computeDescriptorPool);

        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, computeDescriptorPool, 1, &computeDescriptorSetLayout};
        if (vkAllocateDescriptorSets(device->device(), &allocInfo, &computeDescriptorSet) != VK_SUCCESS) {
             throw std::runtime_error("failed to allocate compute descriptor sets!");
        }

        // --- Update Descriptor Set ---
        VkDescriptorBufferInfo instInfo{ instanceBuffer, 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo camInfo{ cameraBuffer, 0, sizeof(CameraData) };
        VkDescriptorBufferInfo indirInfo{ indirectDrawBuffer, 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo visInfo{ visibleInstanceBuffer, 0, VK_WHOLE_SIZE };

        std::vector<VkWriteDescriptorSet> computeWrites;
        computeWrites.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, computeDescriptorSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &instInfo, nullptr});
        computeWrites.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, computeDescriptorSet, 1, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &camInfo, nullptr});
        computeWrites.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, computeDescriptorSet, 2, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &indirInfo, nullptr});
        computeWrites.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, computeDescriptorSet, 3, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &visInfo, nullptr});

        vkUpdateDescriptorSets(device->device(), static_cast<uint32_t>(computeWrites.size()), computeWrites.data(), 0, nullptr);

        VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipelineInfo.layout = computePipelineLayout;
        pipelineInfo.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_COMPUTE_BIT, computeModule, "main", nullptr};

        vkCreateComputePipelines(device->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline);
        vkDestroyShaderModule(device->device(), computeModule, nullptr);
    }

    void InstancingScene::createGraphicsPipeline(VulkanRenderer* renderer) {
        // Set 0: Camera (UBO)
        VkDescriptorSetLayoutBinding camBinding = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo set0Info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1, &camBinding};
        vkCreateDescriptorSetLayout(device->device(), &set0Info, nullptr, &graphicsSet0Layout);

        // Set 1: Instance(SSBO), Visible(SSBO)
        VkDescriptorSetLayoutBinding instBindings[] = {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
        };
        VkDescriptorSetLayoutCreateInfo set1Info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 2, instBindings};
        VkDescriptorSetLayout set1Layout;
        vkCreateDescriptorSetLayout(device->device(), &set1Info, nullptr, &set1Layout);

        graphicsDescriptorSetLayout = set1Layout; 
        std::array<VkDescriptorSetLayout, 2> layouts = { graphicsSet0Layout, graphicsDescriptorSetLayout };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineLayoutInfo.setLayoutCount = 2;
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        vkCreatePipelineLayout(device->device(), &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout);

        // --- Allocation ---
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }
        };
        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr, 0, 1, 2, poolSizes};
        poolInfo.maxSets = 2; // We need 2 sets (Set 0 and Set 1)
        vkCreateDescriptorPool(device->device(), &poolInfo, nullptr, &graphicsDescriptorPool);

        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, graphicsDescriptorPool, 2, layouts.data()};
        graphicsDescriptorSets.resize(2);
        if (vkAllocateDescriptorSets(device->device(), &allocInfo, graphicsDescriptorSets.data()) != VK_SUCCESS) {
             throw std::runtime_error("failed to allocate graphics descriptor sets!");
        }

        // Update Set 0
        VkDescriptorBufferInfo camInfo{ cameraBuffer, 0, sizeof(CameraData) };
        VkWriteDescriptorSet writeCam{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, graphicsDescriptorSets[0], 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &camInfo, nullptr};
        
        // Update Set 1
        VkDescriptorBufferInfo instInfo{ instanceBuffer, 0, VK_WHOLE_SIZE };
        VkDescriptorBufferInfo visInfo{ visibleInstanceBuffer, 0, VK_WHOLE_SIZE };
        VkWriteDescriptorSet writes[] = {
            {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, graphicsDescriptorSets[1], 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &instInfo, nullptr},
            {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, graphicsDescriptorSets[1], 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &visInfo, nullptr}
        };
        
        vkUpdateDescriptorSets(device->device(), 1, &writeCam, 0, nullptr);
        vkUpdateDescriptorSets(device->device(), 2, writes, 0, nullptr);

        // --- Pipeline ---
        auto readFile = [](const std::string& filename) {
             std::vector<std::string> paths = {
                 "build/Debug/" + filename,
                 "../shaders/" + filename,
                 "../../shaders/" + filename,
                 "shaders/" + filename,
                 filename
             };

             for (const auto& path : paths) {
                 std::ifstream file(path, std::ios::ate | std::ios::binary);
                 if (file.is_open()) {
                     size_t fileSize = (size_t)file.tellg();
                     std::vector<char> buffer(fileSize);
                     file.seekg(0);
                     file.read(buffer.data(), fileSize);
                     return buffer;
                 }
             }
             throw std::runtime_error("Failed to find/open shader file: " + filename);
        };

        auto vertCode = readFile("instance.vert.spv");
        auto fragCode = readFile("instance.frag.spv");
        VkShaderModule vertModule, fragModule;
        VkShaderModuleCreateInfo vInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, vertCode.size(), reinterpret_cast<const uint32_t*>(vertCode.data())};
        vkCreateShaderModule(device->device(), &vInfo, nullptr, &vertModule);
        VkShaderModuleCreateInfo fInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0, fragCode.size(), reinterpret_cast<const uint32_t*>(fragCode.data())};
        vkCreateShaderModule(device->device(), &fInfo, nullptr, &fragModule);

        VkPipelineShaderStageCreateInfo stages[] = {
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", nullptr},
            {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", nullptr}
        };

        auto bindDesc = Vertex::getBindingDescription();
        auto attrDesc = Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertInput{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 1, &bindDesc, static_cast<uint32_t>(attrDesc.size()), attrDesc.data()};
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE};
        
        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0, 1, nullptr, 1, nullptr};
        VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0, 0, 0, 1.0f};
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0, VK_SAMPLE_COUNT_1_BIT, VK_FALSE};
        VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, nullptr, 0, VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE};
        
        VkPipelineColorBlendAttachmentState blendAtt{VK_FALSE, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, 0xF};
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &blendAtt};

        std::vector<VkDynamicState> dynamics = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicInfo{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, static_cast<uint32_t>(dynamics.size()), dynamics.data()};

        VkGraphicsPipelineCreateInfo pipeInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeInfo.stageCount = 2;
        pipeInfo.pStages = stages;
        pipeInfo.pVertexInputState = &vertInput;
        pipeInfo.pInputAssemblyState = &inputAssembly;
        pipeInfo.pViewportState = &viewportState;
        pipeInfo.pRasterizationState = &rasterizer;
        pipeInfo.pMultisampleState = &multisample;
        pipeInfo.pDepthStencilState = &depthStencil;
        pipeInfo.pColorBlendState = &blend;
        pipeInfo.pDynamicState = &dynamicInfo;
        pipeInfo.layout = graphicsPipelineLayout;
        pipeInfo.renderPass = renderer->getSwapChainRenderPass();
        pipeInfo.subpass = 0;

        vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &graphicsPipeline);

        vkDestroyShaderModule(device->device(), vertModule, nullptr);
        vkDestroyShaderModule(device->device(), fragModule, nullptr);
    }
    
    void InstancingScene::createDescriptorSets() {
        // Already handled in createComputePipeline and createGraphicsPipeline
        // This function was originally just for Compute, logic moved to initialization for clarity in correct order
    }

    void InstancingScene::onImGuiRender() {
        ImGui::Begin("GPU Instancing Stats");
        ImGui::Text("Total Instances: %d", INSTANCE_COUNT);
        ImGui::Text("Visible Instances: %d (GPU)", visibleCountCheck); 
        ImGui::Checkbox("Freeze Culling", &freezeCulling);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::End();
    }
    
    void InstancingScene::cleanup() {
        if (device) vkDeviceWaitIdle(device->device());

        if (computePipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device->device(), computePipeline, nullptr); computePipeline = VK_NULL_HANDLE; }
        if (computePipelineLayout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device->device(), computePipelineLayout, nullptr); computePipelineLayout = VK_NULL_HANDLE; }
        if (computeDescriptorSetLayout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device->device(), computeDescriptorSetLayout, nullptr); computeDescriptorSetLayout = VK_NULL_HANDLE; }
        if (computeDescriptorPool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device->device(), computeDescriptorPool, nullptr); computeDescriptorPool = VK_NULL_HANDLE; }
        
        if (graphicsPipeline != VK_NULL_HANDLE) { vkDestroyPipeline(device->device(), graphicsPipeline, nullptr); graphicsPipeline = VK_NULL_HANDLE; }
        if (graphicsPipelineLayout != VK_NULL_HANDLE) { vkDestroyPipelineLayout(device->device(), graphicsPipelineLayout, nullptr); graphicsPipelineLayout = VK_NULL_HANDLE; }
        if (graphicsDescriptorSetLayout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device->device(), graphicsDescriptorSetLayout, nullptr); graphicsDescriptorSetLayout = VK_NULL_HANDLE; }
        if (graphicsSet0Layout != VK_NULL_HANDLE) { vkDestroyDescriptorSetLayout(device->device(), graphicsSet0Layout, nullptr); graphicsSet0Layout = VK_NULL_HANDLE; }
        if (graphicsDescriptorPool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device->device(), graphicsDescriptorPool, nullptr); graphicsDescriptorPool = VK_NULL_HANDLE; }

        if (instanceBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device->device(), instanceBuffer, nullptr); instanceBuffer = VK_NULL_HANDLE; }
        if (instanceBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device->device(), instanceBufferMemory, nullptr); instanceBufferMemory = VK_NULL_HANDLE; }
        if (cameraBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device->device(), cameraBuffer, nullptr); cameraBuffer = VK_NULL_HANDLE; }
        if (cameraBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device->device(), cameraBufferMemory, nullptr); cameraBufferMemory = VK_NULL_HANDLE; }
        if (indirectDrawBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device->device(), indirectDrawBuffer, nullptr); indirectDrawBuffer = VK_NULL_HANDLE; }
        if (indirectDrawBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device->device(), indirectDrawBufferMemory, nullptr); indirectDrawBufferMemory = VK_NULL_HANDLE; }
        if (visibleInstanceBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device->device(), visibleInstanceBuffer, nullptr); visibleInstanceBuffer = VK_NULL_HANDLE; }
        if (visibleInstanceBufferMemory != VK_NULL_HANDLE) { vkFreeMemory(device->device(), visibleInstanceBufferMemory, nullptr); visibleInstanceBufferMemory = VK_NULL_HANDLE; }
    }
}
