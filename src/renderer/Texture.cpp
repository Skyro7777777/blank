#include "renderer/Texture.h"
#include "core/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ps {

Texture::~Texture() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_width(other.m_width), m_height(other.m_height),
      m_channels(other.m_channels), m_isHDR(other.m_isHDR),
      m_type(other.m_type), m_path(std::move(other.m_path)) {
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        m_isHDR = other.m_isHDR;
        m_type = other.m_type;
        m_path = std::move(other.m_path);
        other.m_id = 0;
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& path, bool flipVertically) {
    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data) {
        LOG_ERROR("Failed to load texture: %s (%s)", path.c_str(), stbi_failure_reason());
        return false;
    }

    // Determine format
    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA8;
    switch (m_channels) {
        case 1: format = GL_RED; internalFormat = GL_R8; break;
        case 2: format = GL_RG; internalFormat = GL_RG8; break;
        case 3: format = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: format = GL_RGBA; internalFormat = GL_RGBA8; break;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Default parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    m_path = path;
    m_isHDR = false;
    LOG_DEBUG("Loaded texture: %s (%dx%d, %d channels)", path.c_str(), m_width, m_height, m_channels);
    return true;
}

bool Texture::loadFromMemory(const unsigned char* data, unsigned int size, bool flipVertically) {
    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* imgData = stbi_load_from_memory(data, size, &m_width, &m_height, &m_channels, 0);
    if (!imgData) {
        LOG_ERROR("Failed to load embedded texture (%s)", stbi_failure_reason());
        return false;
    }

    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_RGBA8;
    switch (m_channels) {
        case 1: format = GL_RED; internalFormat = GL_R8; break;
        case 2: format = GL_RG; internalFormat = GL_RG8; break;
        case 3: format = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: format = GL_RGBA; internalFormat = GL_RGBA8; break;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, imgData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(imgData);

    m_isHDR = false;
    LOG_DEBUG("Loaded embedded texture (%dx%d, %d channels)", m_width, m_height, m_channels);
    return true;
}

bool Texture::loadHDR(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);

    float* data = stbi_loadf(path.c_str(), &m_width, &m_height, &m_channels, 0);
    if (!data) {
        LOG_ERROR("Failed to load HDR texture: %s (%s)", path.c_str(), stbi_failure_reason());
        return false;
    }

    GLenum format = GL_RGB;
    GLenum internalFormat = GL_RGB16F;
    if (m_channels == 4) {
        format = GL_RGBA;
        internalFormat = GL_RGBA16F;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_FLOAT, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    m_path = path;
    m_isHDR = true;
    LOG_INFO("Loaded HDR texture: %s (%dx%d, %d channels)", path.c_str(), m_width, m_height, m_channels);
    return true;
}

bool Texture::createEmpty(int width, int height, GLenum internalFormat, GLenum format, GLenum type) {
    m_width = width;
    m_height = height;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool Texture::createDepth(int width, int height) {
    m_width = width;
    m_height = height;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindTexture(GL_TEXTURE_2D, 0);
    m_type = TextureType::UNKNOWN; // Used for shadow maps
    return true;
}

bool Texture::createCubemap(int size, GLenum internalFormat) {
    m_width = size;
    m_height = size;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

    GLenum format = GL_RGB;
    if (internalFormat == GL_RGBA16F) format = GL_RGBA;

    for (unsigned int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat,
                     size, size, 0, format, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    m_type = TextureType::CUBEMAP;
    return true;
}

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    if (m_type == TextureType::CUBEMAP) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_id);
    }
}

void Texture::unbind() const {
    if (m_type == TextureType::CUBEMAP) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

TextureType textureTypeFromString(const std::string& name) {
    if (name.find("diffuse") != std::string::npos || name.find("albedo") != std::string::npos ||
        name.find("color") != std::string::npos || name.find("base_color") != std::string::npos)
        return TextureType::DIFFUSE;
    if (name.find("specular") != std::string::npos)
        return TextureType::SPECULAR;
    if (name.find("normal") != std::string::npos || name.find("norm") != std::string::npos)
        return TextureType::NORMAL;
    if (name.find("roughness") != std::string::npos || name.find("rough") != std::string::npos)
        return TextureType::ROUGHNESS;
    if (name.find("metallic") != std::string::npos || name.find("metal") != std::string::npos)
        return TextureType::METALLIC;
    if (name.find("ao") != std::string::npos || name.find("ambient") != std::string::npos ||
        name.find("occlusion") != std::string::npos)
        return TextureType::AO;
    if (name.find("emission") != std::string::npos || name.find("emissive") != std::string::npos)
        return TextureType::EMISSION;
    if (name.find("height") != std::string::npos || name.find("displacement") != std::string::npos)
        return TextureType::HEIGHT;
    return TextureType::UNKNOWN;
}

} // namespace ps
