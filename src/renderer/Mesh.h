#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace ps {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

// Material properties for a mesh
struct MeshMaterial {
    // Texture IDs (0 = not set)
    unsigned int diffuseMap = 0;
    unsigned int normalMap = 0;
    unsigned int roughnessMap = 0;
    unsigned int metallicMap = 0;
    unsigned int aoMap = 0;
    unsigned int emissionMap = 0;

    // Scalar material properties (fallback when no texture)
    glm::vec3 albedo{0.8f};
    float roughness = 0.5f;
    float metallic = 0.0f;
    float ao = 1.0f;
    glm::vec3 emission{0.0f};
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Initialize with vertex/index data
    void setup(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

    // Draw the mesh
    void draw() const;

    // Accessors
    const std::vector<Vertex>& vertices() const { return m_vertices; }
    const std::vector<unsigned int>& indices() const { return m_indices; }
    MeshMaterial& material() { return m_material; }
    const MeshMaterial& material() const { return m_material; }
    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

private:
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_ebo = 0;

    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    MeshMaterial m_material;
    std::string m_name;
};

} // namespace ps
