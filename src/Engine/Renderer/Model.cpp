#include "Model.h"
#include <iostream>
#include <stdexcept>

namespace AhnrealEngine {

Model::Model(VulkanDevice *device, const std::string &path) : device(device) {
  loadModel(path);
}

Model::~Model() {
    // Meshes are managed by unique_ptr, so they will be automatically cleaned up.
}

void Model::draw(VkCommandBuffer commandBuffer) {
  for (const auto &mesh : meshes) {
    mesh->draw(commandBuffer);
  }
}

void Model::loadModel(const std::string &path) {
  Assimp::Importer importer;
  // Presets: Triangulate, FlipUVs for Vulkan/OpenGL, CalcTangentSpace for normal mapping
  // Note: Vulkan GLSL usually wants FlipUVs, but depending on the pipeline setup.
  // We'll stick to standard settings.
  const aiScene *scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_FlipUVs |
                aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals |
                aiProcess_JoinIdenticalVertices);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throw std::runtime_error("Failed to load model (" + path +
                             "): " + importer.GetErrorString());
  }

  // Extract directory path for texture loading
  directory = path.substr(0, path.find_last_of('/'));
  if (directory == path) { // Handle Windows backslash case or no path
      directory = path.substr(0, path.find_last_of('\\'));
      if (directory == path) directory = "";
  }

  processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
  // Process all meshes at this node
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    meshes.push_back(std::make_unique<Mesh>(processMesh(mesh, scene)));
  }

  // Recursively process children
  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    processNode(node->mChildren[i], scene);
  }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  // Process vertices
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex{};

    // Position
    vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                       mesh->mVertices[i].z};

    // Normal
    if (mesh->HasNormals()) {
      vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                       mesh->mNormals[i].z};
    }

    // TexCoords
    if (mesh->mTextureCoords[0]) {
      vertex.texCoord = {mesh->mTextureCoords[0][i].x,
                         mesh->mTextureCoords[0][i].y};
    } else {
      vertex.texCoord = {0.0f, 0.0f};
    }
    
    // Tangent / Bitangent
    if (mesh->HasTangentsAndBitangents()) {
        vertex.tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z};
        vertex.bitangent = {mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z};
    }

    vertices.push_back(vertex);
  }

  // Process indices
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  // TODO: Process materials here

  return Mesh(device, vertices, indices);
}

} // namespace AhnrealEngine
