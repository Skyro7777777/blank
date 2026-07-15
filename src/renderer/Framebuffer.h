#pragma once

#include <glad/gl.h>

namespace ps {

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // Create shadow map framebuffer
    bool createShadowMap(int width, int height);

    // Bind/unbind
    void bind() const;
    void unbind() const;

    // Get depth texture
    unsigned int depthTexture() const { return m_depthTexture; }

    // Get dimensions
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    unsigned int m_fbo = 0;
    unsigned int m_depthTexture = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace ps
