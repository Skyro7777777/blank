#pragma once

#include <glad/gl.h>
#include <string>
#include <vector>

namespace ps {

enum class TextureType {
    DIFFUSE,
    SPECULAR,
    NORMAL,
    ROUGHNESS,
    METALLIC,
    AO,
    EMISSION,
    HEIGHT,
    HDR,         // HDR equirectangular
    CUBEMAP,     // Cubemap texture
    UNKNOWN
};

class Texture {
public:
    Texture() = default;
    ~Texture();

    // Prevent copying (GPU resource)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Load a 2D texture from file
    bool loadFromFile(const std::string& path, bool flipVertically = true);

    // Load a 2D texture from memory (embedded in GLB)
    bool loadFromMemory(const unsigned char* data, unsigned int size, bool flipVertically = true);

    // Load HDR texture (float data)
    bool loadHDR(const std::string& path);

    // Create an empty 2D texture with given parameters
    bool createEmpty(int width, int height, GLenum internalFormat = GL_RGBA8,
                     GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);

    // Create depth texture (for shadow mapping)
    bool createDepth(int width, int height);

    // Create empty cubemap
    bool createCubemap(int size, GLenum internalFormat = GL_RGBA16F);

    // Bind to a texture unit
    void bind(unsigned int unit = 0) const;

    // Unbind
    void unbind() const;

    // Getters
    unsigned int id() const { return m_id; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool isHDR() const { return m_isHDR; }
    TextureType type() const { return m_type; }
    const std::string& path() const { return m_path; }

    void setType(TextureType type) { m_type = type; }

private:
    unsigned int m_id = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;
    bool m_isHDR = false;
    TextureType m_type = TextureType::UNKNOWN;
    std::string m_path;
};

// Utility: get texture type from string name
TextureType textureTypeFromString(const std::string& name);

} // namespace ps
