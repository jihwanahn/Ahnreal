#include "CameraTestScene.h"
#include "../../Engine/Core/Input.h"
#include "../../Engine/Renderer/VulkanDevice.h"
#include "../../Engine/Renderer/VulkanRenderer.h"
#include <cstring>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <stdexcept>
#include <vector>


namespace AhnrealEngine {

VkVertexInputBindingDescription CameraTestVertex::getBindingDescription() {
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(CameraTestVertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription>
CameraTestVertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(CameraTestVertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(CameraTestVertex, color);

  return attributeDescriptions;
}

CameraTestScene::CameraTestScene()
    : Scene("Camera Test Scene"), camera(glm::vec3(0.0f, 5.0f, 10.0f)) {
  camera.setPitch(-20.0f);
}

CameraTestScene::~CameraTestScene() { cleanup(); }

void CameraTestScene::initialize() {
  // Empty - use initialize(renderer) instead
}

void CameraTestScene::initialize(VulkanRenderer *renderer) {
  device = renderer->getDevice();

  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffers();
  createDescriptorSetLayout();
  createDescriptorPool();
  createDescriptorSets();
  createGraphicsPipeline(renderer);
}

void CameraTestScene::update(float deltaTime) {
  // Process camera input
  if (Input::isKeyPressed(GLFW_KEY_W)) {
    camera.processKeyboard(CameraMovement::Forward, deltaTime);
  }
  if (Input::isKeyPressed(GLFW_KEY_S)) {
    camera.processKeyboard(CameraMovement::Backward, deltaTime);
  }
  if (Input::isKeyPressed(GLFW_KEY_A)) {
    camera.processKeyboard(CameraMovement::Left, deltaTime);
  }
  if (Input::isKeyPressed(GLFW_KEY_D)) {
    camera.processKeyboard(CameraMovement::Right, deltaTime);
  }
  if (Input::isKeyPressed(GLFW_KEY_SPACE)) {
    camera.processKeyboard(CameraMovement::Up, deltaTime);
  }
  if (Input::isKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
    camera.processKeyboard(CameraMovement::Down, deltaTime);
  }

  // Mouse look when right button held
  if (Input::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
    glm::vec2 delta = Input::getMouseDelta();
    camera.processMouseMovement(delta.x, delta.y);
  }

  // Handle scroll for zoom/orbit distance
  float scroll = Input::getScrollDelta();
  if (scroll != 0.0f) {
    camera.processMouseScroll(scroll);
  }

  // Toggle camera mode with Tab
  if (Input::isKeyJustPressed(GLFW_KEY_TAB)) {
    if (camera.getMode() == CameraMode::FreeCamera) {
      camera.setMode(CameraMode::Orbit);
    } else {
      camera.setMode(CameraMode::FreeCamera);
    }
  }

  // Update camera settings
  camera.setMovementSpeed(cameraSpeed);
  camera.setMouseSensitivity(mouseSensitivity);
}

void CameraTestScene::render(VulkanRenderer *renderer) {
  if (!device || graphicsPipeline == VK_NULL_HANDLE)
    return;

  VkCommandBuffer commandBuffer = renderer->getCurrentCommandBuffer();
  uint32_t currentFrame = renderer->getFrameIndex();
  VkExtent2D extent = renderer->getSwapChainExtent();

  // Choose pipeline based on wireframe mode
  VkPipeline currentPipeline =
      wireframeMode ? wireframePipeline : graphicsPipeline;
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    currentPipeline);

  VkBuffer vertexBuffers[] = {vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

  // Render grid of cubes
  int cubeIndex = 0;
  for (int x = -gridSize / 2; x <= gridSize / 2; x++) {
    for (int z = -gridSize / 2; z <= gridSize / 2; z++) {
      updateUniformBuffer(cubeIndex);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipelineLayout, 0, 1,
                              &descriptorSets[currentFrame], 0, nullptr);

      // Update model matrix for this cube position
      CameraTestUBO ubo{};
      ubo.model = glm::translate(
          glm::mat4(1.0f), glm::vec3(x * gridSpacing, 0.0f, z * gridSpacing));

      float aspectRatio =
          static_cast<float>(extent.width) / static_cast<float>(extent.height);
      ubo.view = camera.getViewMatrix();
      ubo.proj = camera.getProjectionMatrix(aspectRatio);

      memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));

      vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1,
                       0, 0, 0);
      cubeIndex++;
    }
  }
}

void CameraTestScene::cleanup() {
  if (device) {
    vkDeviceWaitIdle(device->device());

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
      vkDestroyDescriptorSetLayout(device->device(), descriptorSetLayout,
                                   nullptr);
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

void CameraTestScene::onImGuiRender() {
  ImGui::Begin("Camera Test Controls");

  ImGui::Text("Camera Test Scene");
  ImGui::Separator();

  // Camera info
  ImGui::Text("Camera Mode: %s", camera.getMode() == CameraMode::FreeCamera
                                     ? "Free Camera"
                                     : "Orbit");

  glm::vec3 pos = camera.getPosition();
  ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
  ImGui::Text("Yaw: %.2f, Pitch: %.2f", camera.getYaw(), camera.getPitch());
  ImGui::Text("FOV: %.1f", camera.getZoom());

  ImGui::Separator();

  // Camera settings
  ImGui::Text("Camera Settings:");
  ImGui::SliderFloat("Movement Speed", &cameraSpeed, 1.0f, 20.0f);
  ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 1.0f);

  if (ImGui::Button("Reset Camera")) {
    camera.reset();
    camera.setPosition(glm::vec3(0.0f, 5.0f, 10.0f));
    camera.setPitch(-20.0f);
  }

  ImGui::Separator();

  // Grid settings
  ImGui::Text("Grid Settings:");
  ImGui::SliderInt("Grid Size", &gridSize, 1, 10);
  ImGui::SliderFloat("Grid Spacing", &gridSpacing, 1.0f, 5.0f);

  ImGui::Separator();

  // Rendering settings
  ImGui::Checkbox("Wireframe Mode", &wireframeMode);
  ImGui::Checkbox("Show Grid", &showGrid);

  ImGui::Separator();

  // Controls help
  ImGui::Text("Controls:");
  ImGui::BulletText("WASD - Move camera");
  ImGui::BulletText("Space/Shift - Up/Down");
  ImGui::BulletText("Right Mouse + Move - Look around");
  ImGui::BulletText("Scroll - Zoom / Orbit distance");
  ImGui::BulletText("Tab - Toggle camera mode");

  ImGui::End();
}

void CameraTestScene::createVertexBuffer() {
  VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(device->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(device->device(), stagingBufferMemory);

  device->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

  device->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

  vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
  vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
}

void CameraTestScene::createIndexBuffer() {
  VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuffer, stagingBufferMemory);

  void *data;
  vkMapMemory(device->device(), stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device->device(), stagingBufferMemory);

  device->createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

  device->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

  vkDestroyBuffer(device->device(), stagingBuffer, nullptr);
  vkFreeMemory(device->device(), stagingBufferMemory, nullptr);
}

void CameraTestScene::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(CameraTestUBO);

  size_t maxFramesInFlight = 2;
  uniformBuffers.resize(maxFramesInFlight);
  uniformBuffersMemory.resize(maxFramesInFlight);
  uniformBuffersMapped.resize(maxFramesInFlight);

  for (size_t i = 0; i < maxFramesInFlight; i++) {
    device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i], uniformBuffersMemory[i]);

    vkMapMemory(device->device(), uniformBuffersMemory[i], 0, bufferSize, 0,
                &uniformBuffersMapped[i]);
  }
}

void CameraTestScene::createDescriptorSetLayout() {
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

  if (vkCreateDescriptorSetLayout(device->device(), &layoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void CameraTestScene::createDescriptorPool() {
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = static_cast<uint32_t>(2);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = static_cast<uint32_t>(2);

  if (vkCreateDescriptorPool(device->device(), &poolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void CameraTestScene::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(2, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(2);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(2);
  if (vkAllocateDescriptorSets(device->device(), &allocInfo,
                               descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < 2; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(CameraTestUBO);

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

void CameraTestScene::updateUniformBuffer(int cubeIndex) {
  // This is handled directly in render() for per-cube updates
}

void CameraTestScene::createGraphicsPipeline(VulkanRenderer *renderer) {
  if (!device)
    return;

  // Use the same cube shaders
  auto vertShaderCode = readFile("shaders/cube.vert.spv");
  auto fragShaderCode = readFile("shaders/cube.frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto bindingDescription = CameraTestVertex::getBindingDescription();
  auto attributeDescriptions = CameraTestVertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
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
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;

  if (vkCreatePipelineLayout(device->device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
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

  if (vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create solid graphics pipeline!");
  }

  // Create wireframe pipeline
  VkPipelineRasterizationStateCreateInfo wireframeRasterizer = rasterizer;
  wireframeRasterizer.polygonMode = VK_POLYGON_MODE_LINE;

  pipelineInfo.pRasterizationState = &wireframeRasterizer;

  if (vkCreateGraphicsPipelines(device->device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &wireframePipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create wireframe graphics pipeline!");
  }

  vkDestroyShaderModule(device->device(), fragShaderModule, nullptr);
  vkDestroyShaderModule(device->device(), vertShaderModule, nullptr);
}

VkShaderModule
CameraTestScene::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device->device(), &createInfo, nullptr,
                           &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

std::vector<char> CameraTestScene::readFile(const std::string &filename) {
  std::vector<std::string> pathsToTry = {filename,
                                         "Debug/" + filename,
                                         "Release/" + filename,
                                         "build/Debug/" + filename,
                                         "../" + filename,
                                         "./" + filename};

  std::ifstream file;
  std::string actualPath;

  for (const auto &path : pathsToTry) {
    file.open(path, std::ios::ate | std::ios::binary);
    if (file.is_open()) {
      actualPath = path;
      break;
    }
  }

  if (!file.is_open()) {
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
