#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <unordered_map>

namespace ps {

class Shader {
public:
    Shader() = default;
    ~Shader();

    // Load shader from vertex + fragment file paths
    bool load(const std::string& vertexPath, const std::string& fragmentPath);

    // Load shader from vertex + geometry + fragment file paths
    bool load(const std::string& vertexPath, const std::string& geometryPath,
              const std::string& fragmentPath);

    // Use this shader program
    void use() const;

    // Get the shader program ID
    unsigned int id() const { return m_program; }

    // ── Uniform setters ──
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    unsigned int m_program = 0;
    mutable std::unordered_map<std::string, int> m_uniformCache;

    // Compile a shader stage from source
    unsigned int compileStage(GLenum type, const std::string& source, const std::string& path) const;

    // Link shader program
    bool linkProgram(unsigned int vertex, unsigned int fragment);
    bool linkProgram(unsigned int vertex, unsigned int geometry, unsigned int fragment);

    // Read file contents
    static std::string readFile(const std::string& path);

    // Get uniform location (cached)
    int getUniformLocation(const std::string& name) const;
};

} // namespace ps
