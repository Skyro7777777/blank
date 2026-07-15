#include "renderer/Skybox.h"
#include "core/Logger.h"

// stb_image already implemented in Texture.cpp
#undef STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ps {

static const float skyboxVertices[] = {
    -1, 1,-1, -1,-1,-1,  1,-1,-1,  1,-1,-1,  1, 1,-1, -1, 1,-1,
    -1,-1, 1, -1,-1,-1, -1, 1,-1, -1, 1,-1, -1, 1, 1, -1,-1, 1,
     1,-1,-1,  1,-1, 1,  1, 1, 1,  1, 1, 1,  1, 1,-1,  1,-1,-1,
    -1,-1, 1, -1, 1, 1,  1, 1, 1,  1, 1, 1,  1,-1, 1, -1,-1, 1,
    -1, 1,-1,  1, 1,-1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1, 1,-1,
    -1,-1,-1, -1,-1, 1,  1,-1,-1,  1,-1,-1, -1,-1, 1,  1,-1, 1
};

static const char* equirectVertSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 WorldPos;
uniform mat4 projection;
uniform mat4 view;
void main() {
    WorldPos = aPos;
    gl_Position = projection * view * vec4(WorldPos, 1.0);
}
)";

static const char* equirectFragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 WorldPos;
uniform sampler2D equirectangularMap;
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
void main() {
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}
)";

static const char* skyboxVertSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 projection;
uniform mat4 view;
void main() {
    TexCoords = aPos;
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

static const char* skyboxFragSrc = R"(
#version 330 core
out vec4 FragColor;
in vec3 TexCoords;
uniform samplerCube skybox;
uniform float exposure;
void main() {
    vec3 color = texture(skybox, TexCoords).rgb;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
)";

static unsigned int compileS(GLenum type, const char* source) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        LOG_ERROR("Skybox shader error: %s", log);
        return 0;
    }
    return s;
}

static unsigned int linkProg(unsigned int v, unsigned int f) {
    unsigned int p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    int ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        LOG_ERROR("Skybox link error: %s", log);
        glDeleteProgram(p);
        return 0;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

Skybox::~Skybox() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_cubemapID) glDeleteTextures(1, &m_cubemapID);
    if (m_captureFBO) glDeleteFramebuffers(1, &m_captureFBO);
    if (m_captureRBO) glDeleteRenderbuffers(1, &m_captureRBO);
    if (m_equirectToCubeShader) glDeleteProgram(m_equirectToCubeShader);
    if (m_skyboxShader) glDeleteProgram(m_skyboxShader);
}

bool Skybox::loadHDR(const std::string& hdrPath, int cubemapSize) {
    stbi_set_flip_vertically_on_load(true);
    int w, h, channels;
    float* data = stbi_loadf(hdrPath.c_str(), &w, &h, &channels, 0);
    if (!data) {
        LOG_ERROR("Failed to load HDR: %s (%s)", hdrPath.c_str(), stbi_failure_reason());
        return false;
    }

    unsigned int hdrTex;
    glGenTextures(1, &hdrTex);
    glBindTexture(GL_TEXTURE_2D, hdrTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    LOG_INFO("Loaded HDR: %s (%dx%d)", hdrPath.c_str(), w, h);

    if (!generateCubemap(hdrTex, cubemapSize)) {
        glDeleteTextures(1, &hdrTex);
        return false;
    }
    glDeleteTextures(1, &hdrTex);

    // Create skybox VAO/VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // Compile skybox render shader
    unsigned int sv = compileS(GL_VERTEX_SHADER, skyboxVertSrc);
    unsigned int sf = compileS(GL_FRAGMENT_SHADER, skyboxFragSrc);
    if (sv && sf) m_skyboxShader = linkProg(sv, sf);

    m_loaded = true;
    LOG_INFO("Skybox ready (%dx%d per face)", cubemapSize, cubemapSize);
    return true;
}

bool Skybox::generateCubemap(unsigned int hdrTexture, int size) {
    unsigned int v = compileS(GL_VERTEX_SHADER, equirectVertSrc);
    unsigned int f = compileS(GL_FRAGMENT_SHADER, equirectFragSrc);
    if (!v || !f) return false;
    m_equirectToCubeShader = linkProg(v, f);
    if (!m_equirectToCubeShader) return false;

    glGenTextures(1, &m_cubemapID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapID);
    for (unsigned int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     size, size, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &m_captureFBO);
    glGenRenderbuffers(1, &m_captureRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_captureRBO);

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[] = {
        glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
    };

    glUseProgram(m_equirectToCubeShader);
    int projLoc = glGetUniformLocation(m_equirectToCubeShader, "projection");
    int viewLoc = glGetUniformLocation(m_equirectToCubeShader, "view");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glUniform1i(glGetUniformLocation(m_equirectToCubeShader, "equirectangularMap"), 0);

    glViewport(0, 0, size, size);
    glBindFramebuffer(GL_FRAMEBUFFER, m_captureFBO);

    unsigned int tmpVAO, tmpVBO;
    glGenVertexArrays(1, &tmpVAO);
    glGenBuffers(1, &tmpVBO);
    glBindVertexArray(tmpVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    for (unsigned int i = 0; i < 6; i++) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(views[i]));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_cubemapID, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteVertexArrays(1, &tmpVAO);
    glDeleteBuffers(1, &tmpVBO);

    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapID);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    return true;
}

void Skybox::draw(const glm::mat4& view, const glm::mat4& projection) const {
    if (!m_loaded || !m_skyboxShader) return;

    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glUseProgram(m_skyboxShader);
    glUniformMatrix4fv(glGetUniformLocation(m_skyboxShader, "view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_skyboxShader, "projection"),
                       1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(m_skyboxShader, "exposure"), exposure);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapID);
    glUniform1i(glGetUniformLocation(m_skyboxShader, "skybox"), 0);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

} // namespace ps
