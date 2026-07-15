#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

namespace ps {

class Skybox {
public:
    Skybox() = default;
    ~Skybox();

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;

    // Load HDR equirectangular map and convert to cubemap
    bool loadHDR(const std::string& hdrPath, int cubemapSize = 512);

    // Draw the skybox
    void draw(const glm::mat4& view, const glm::mat4& projection) const;

    // Get cubemap texture ID
    unsigned int cubemapTexture() const { return m_cubemapID; }
    bool isLoaded() const { return m_loaded; }

    // Exposure for HDR tone mapping
    float exposure = 1.0f;

private:
    bool generateCubemap(unsigned int hdrTexture, int size);

    // Skybox VAO/VBO
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;

    // Cubemap
    unsigned int m_cubemapID = 0;

    // Capture FBO
    unsigned int m_captureFBO = 0;
    unsigned int m_captureRBO = 0;

    // Shaders
    unsigned int m_equirectToCubeShader = 0;
    unsigned int m_skyboxShader = 0;

    bool m_loaded = false;
};

} // namespace ps
