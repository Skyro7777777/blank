#include "renderer/Model.h"
#include "core/Logger.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <cstdlib>

namespace ps {

bool Model::load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes |
        aiProcess_ImproveCacheLocality
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG_ERROR("Assimp error loading model: %s - %s", path.c_str(), importer.GetErrorString());
        return false;
    }

    m_path = path;
    m_directory = std::filesystem::path(path).parent_path().string();
    m_scene = scene; // Store for embedded texture access

    processNode(scene->mRootNode, scene);

    LOG_INFO("Model loaded: %s (%zu meshes, %zu textures)",
             path.c_str(), m_meshes.size(), m_textures.size());
    return true;
}

void Model::draw() const {
    for (const auto& mesh : m_meshes) {
        mesh.draw();
    }
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        m_meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* aiMesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Vertices
    vertices.reserve(aiMesh->mNumVertices);
    for (unsigned int i = 0; i < aiMesh->mNumVertices; i++) {
        Vertex vertex{};

        vertex.position = { aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z };

        if (aiMesh->mNormals) {
            vertex.normal = { aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z };
        }

        if (aiMesh->mTextureCoords[0]) {
            vertex.texCoords = { aiMesh->mTextureCoords[0][i].x, aiMesh->mTextureCoords[0][i].y };
        }

        if (aiMesh->mTangents) {
            vertex.tangent = { aiMesh->mTangents[i].x, aiMesh->mTangents[i].y, aiMesh->mTangents[i].z };
        }

        if (aiMesh->mBitangents) {
            vertex.bitangent = { aiMesh->mBitangents[i].x, aiMesh->mBitangents[i].y, aiMesh->mBitangents[i].z };
        }

        vertices.push_back(vertex);
    }

    // Indices
    for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
        aiFace& face = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh mesh;
    mesh.setup(vertices, indices);
    mesh.setName(aiMesh->mName.C_Str());

    // Process material
    if (aiMesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
        loadMaterialTextures(material, mesh.material());
    }

    return mesh;
}

void Model::loadMaterialTextures(aiMaterial* mat, MeshMaterial& meshMat) {
    // Helper to load first texture of a given Assimp type
    auto loadTex = [&](aiTextureType aiType, TextureType texType) -> unsigned int {
        if (mat->GetTextureCount(aiType) > 0) {
            aiString aiPath;
            mat->GetTexture(aiType, 0, &aiPath);
            std::string pathStr = aiPath.C_Str();

            // Check if it's an embedded texture (*N format)
            if (!pathStr.empty() && pathStr[0] == '*') {
                int texIndex = std::atoi(pathStr.c_str() + 1);
                if (texIndex >= 0 && texIndex < (int)m_scene->mNumTextures) {
                    const aiTexture* aiTex = m_scene->mTextures[texIndex];
                    if (aiTex && aiTex->mHeight == 0) {
                        // Compressed embedded texture (PNG/JPG data)
                        std::string cacheKey = "embedded_" + pathStr;
                        auto it = m_textureCache.find(cacheKey);
                        if (it != m_textureCache.end()) return it->second;

                        Texture tex;
                        if (tex.loadFromMemory((const unsigned char*)aiTex->pcData, aiTex->mWidth)) {
                            tex.setType(texType);
                            unsigned int id = tex.id();
                            m_textureCache[cacheKey] = id;
                            m_textures.push_back(std::move(tex));
                            return id;
                        }
                    }
                }
                return 0;
            }

            // External texture file
            std::string fullPath = m_directory + "/" + pathStr;

            // Check cache
            auto it = m_textureCache.find(fullPath);
            if (it != m_textureCache.end()) {
                return it->second;
            }

            Texture tex;
            if (tex.loadFromFile(fullPath)) {
                tex.setType(texType);
                unsigned int id = tex.id();
                m_textureCache[fullPath] = id;
                m_textures.push_back(std::move(tex));
                return id;
            }
        }
        return 0;
    };

    meshMat.diffuseMap = loadTex(aiTextureType_DIFFUSE, TextureType::DIFFUSE);
    meshMat.normalMap = loadTex(aiTextureType_NORMALS, TextureType::NORMAL);
    meshMat.roughnessMap = loadTex(aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS);
    if (!meshMat.roughnessMap) {
        meshMat.roughnessMap = loadTex(aiTextureType_SPECULAR, TextureType::ROUGHNESS);
    }
    meshMat.metallicMap = loadTex(aiTextureType_METALNESS, TextureType::METALLIC);
    meshMat.aoMap = loadTex(aiTextureType_AMBIENT_OCCLUSION, TextureType::AO);
    if (!meshMat.aoMap) {
        meshMat.aoMap = loadTex(aiTextureType_AMBIENT, TextureType::AO);
    }
    meshMat.emissionMap = loadTex(aiTextureType_EMISSIVE, TextureType::EMISSION);

    // Extract scalar material properties from Assimp
    aiColor3D diffuseColor(0.8f, 0.8f, 0.8f);
    mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
    meshMat.albedo = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);

    float roughness = 0.5f;
    mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
    meshMat.roughness = roughness;

    float metallic = 0.0f;
    mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
    meshMat.metallic = metallic;
}

} // namespace ps
