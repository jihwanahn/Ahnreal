#pragma once

#include "Mesh.h"
#include "VulkanDevice.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <memory>
#include <string>
#include <vector>

namespace AhnrealEngine {

class Model {
public:
  Model(VulkanDevice *device, const std::string &path);
  ~Model();

  // Prevent copying to avoid resource management issues
  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;
  
  // Allow moving
  Model(Model&&) = default;
  Model& operator=(Model&&) = default;

  void draw(VkCommandBuffer commandBuffer);

    const std::vector<std::unique_ptr<Mesh>>& getMeshes() const { return meshes; }

private:
  void loadModel(const std::string &path);
  void processNode(aiNode *node, const aiScene *scene);
  Mesh processMesh(aiMesh *mesh, const aiScene *scene);

  VulkanDevice *device;
  std::vector<std::unique_ptr<Mesh>> meshes;
  std::string directory;
};

} // namespace AhnrealEngine
