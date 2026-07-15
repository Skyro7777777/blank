#include "renderer/Shader.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>

namespace ps {

Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
    }
}

bool Shader::load(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSrc = readFile(vertexPath);
    std::string fragmentSrc = readFile(fragmentPath);

    if (vertexSrc.empty() || fragmentSrc.empty()) return false;

    unsigned int vertex = compileStage(GL_VERTEX_SHADER, vertexSrc, vertexPath);
    unsigned int fragment = compileStage(GL_FRAGMENT_SHADER, fragmentSrc, fragmentPath);

    if (!vertex || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return false;
    }

    bool success = linkProgram(vertex, fragment);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (success) {
        LOG_INFO("Shader loaded: %s", vertexPath.c_str());
    }
    return success;
}

bool Shader::load(const std::string& vertexPath, const std::string& geometryPath,
                  const std::string& fragmentPath) {
    std::string vertexSrc = readFile(vertexPath);
    std::string geometrySrc = readFile(geometryPath);
    std::string fragmentSrc = readFile(fragmentPath);

    if (vertexSrc.empty() || geometrySrc.empty() || fragmentSrc.empty()) return false;

    unsigned int vertex = compileStage(GL_VERTEX_SHADER, vertexSrc, vertexPath);
    unsigned int geometry = compileStage(GL_GEOMETRY_SHADER, geometrySrc, geometryPath);
    unsigned int fragment = compileStage(GL_FRAGMENT_SHADER, fragmentSrc, fragmentPath);

    if (!vertex || !geometry || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (geometry) glDeleteShader(geometry);
        if (fragment) glDeleteShader(fragment);
        return false;
    }

    bool success = linkProgram(vertex, geometry, fragment);

    glDeleteShader(vertex);
    glDeleteShader(geometry);
    glDeleteShader(fragment);

    return success;
}

void Shader::use() const {
    glUseProgram(m_program);
}

// ── Uniform setters ──
void Shader::setBool(const std::string& name, bool value) const {
    glUniform1i(getUniformLocation(name), value ? 1 : 0);
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setMat3(const std::string& name, const glm::mat3& value) const {
    glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

// ── Private methods ──
unsigned int Shader::compileStage(GLenum type, const std::string& source, const std::string& path) const {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);

        const char* typeName = "UNKNOWN";
        switch (type) {
            case GL_VERTEX_SHADER:   typeName = "VERTEX"; break;
            case GL_FRAGMENT_SHADER: typeName = "FRAGMENT"; break;
            case GL_GEOMETRY_SHADER: typeName = "GEOMETRY"; break;
        }
        LOG_ERROR("Shader compile error [%s] %s:\n%s", typeName, path.c_str(), infoLog);
        return 0;
    }
    return shader;
}

bool Shader::linkProgram(unsigned int vertex, unsigned int fragment) {
    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glLinkProgram(m_program);

    int success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(m_program, sizeof(infoLog), nullptr, infoLog);
        LOG_ERROR("Shader link error:\n%s", infoLog);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }
    return true;
}

bool Shader::linkProgram(unsigned int vertex, unsigned int geometry, unsigned int fragment) {
    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, geometry);
    glAttachShader(m_program, fragment);
    glLinkProgram(m_program);

    int success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[1024];
        glGetProgramInfoLog(m_program, sizeof(infoLog), nullptr, infoLog);
        LOG_ERROR("Shader link error:\n%s", infoLog);
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }
    return true;
}

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open shader file: %s", path.c_str());
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int Shader::getUniformLocation(const std::string& name) const {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) return it->second;
    int loc = glGetUniformLocation(m_program, name.c_str());
    m_uniformCache[name] = loc;
    return loc;
}

} // namespace ps
