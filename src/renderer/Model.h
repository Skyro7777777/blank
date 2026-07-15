#pragma once

#include "renderer/Mesh.h"
#include "renderer/Texture.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

// Forward declarations for Assimp
struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;

namespace ps {

class Model {
public:
    Model() = default;
    ~Model() = default;

    bool load(const std::string& path);
    void draw() const;

    const std::vector<Mesh>& meshes() const { return m_meshes; }
    const std::string& directory() const { return m_directory; }
    const std::string& path() const { return m_path; }

    glm::mat4 transform() const { return m_transform; }
    void setTransform(const glm::mat4& t) { m_transform = t; }

private:
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    void loadMaterialTextures(aiMaterial* material, MeshMaterial& mat);

    std::vector<Mesh> m_meshes;
    std::vector<Texture> m_textures;
    std::string m_directory;
    std::string m_path;
    const aiScene* m_scene = nullptr;  // Non-owning, for embedded textures
    glm::mat4 m_transform{1.0f};

    std::unordered_map<std::string, unsigned int> m_textureCache;
};

} // namespace ps
