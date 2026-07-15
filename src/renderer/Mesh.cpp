#include "renderer/Mesh.h"
#include <utility>

namespace ps {

Mesh::~Mesh() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo),
      m_vertices(std::move(other.m_vertices)), m_indices(std::move(other.m_indices)),
      m_material(std::move(other.m_material)), m_name(std::move(other.m_name)) {
    other.m_vao = 0; other.m_vbo = 0; other.m_ebo = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (m_vao) { glDeleteVertexArrays(1, &m_vao); glDeleteBuffers(1, &m_vbo); glDeleteBuffers(1, &m_ebo); }
        m_vao = other.m_vao; m_vbo = other.m_vbo; m_ebo = other.m_ebo;
        m_vertices = std::move(other.m_vertices);
        m_indices = std::move(other.m_indices);
        m_material = std::move(other.m_material);
        m_name = std::move(other.m_name);
        other.m_vao = 0; other.m_vbo = 0; other.m_ebo = 0;
    }
    return *this;
}

void Mesh::setup(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices) {
    m_vertices = vertices;
    m_indices = indices;

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
                 m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_indices.size() * sizeof(unsigned int)),
                 m_indices.data(), GL_STATIC_DRAW);

    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    // TexCoords (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, texCoords));
    // Tangent (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, tangent));
    // Bitangent (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, bitangent));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace ps
