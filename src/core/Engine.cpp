#include "core/Engine.h"
#include "core/Logger.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <algorithm>
#include <cmath>

namespace ps {

Engine::Engine() = default;

Engine::~Engine() {
    m_player.destroy(m_physics);
    for (auto& v : m_vehicles) v->destroy(m_physics);
    for (auto* body : m_staticBodies) {
        m_physics.removeRigidBody(body);
        delete body->getMotionState();
        delete body;
    }
    for (auto* shape : m_ownedShapes) delete shape;

    if (m_running || m_window.handle()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}

bool Engine::init(const Window::Config& config) {
    LOG_INFO("=== Phantom Strike Engine v0.4 - Phase 4 (FPS Mechanics) ===");

    if (!m_window.create(config)) return false;

    int gladVersion = gladLoadGL(glfwGetProcAddress);
    if (!gladVersion) { LOG_FATAL("Failed to initialize GLAD"); return false; }
    LOG_INFO("GLAD loaded OpenGL %d.%d", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));
    LOG_INFO("OpenGL %s | GPU: %s", (const char*)glGetString(GL_VERSION), (const char*)glGetString(GL_RENDERER));

    glfwSetWindowUserPointer(m_window.handle(), &m_input);
    m_input.registerCallbacks(m_window.handle());
    m_input.setCursorCaptured(m_cursorCaptured);

    m_timer = Timer();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window.handle(), true);
    ImGui_ImplOpenGL3_Init(m_glslVersion);

    m_shadowFBO.createShadowMap(SHADOW_WIDTH, SHADOW_HEIGHT);

    m_dirLight.direction = glm::vec3(-0.5f, -1.0f, -0.3f);
    m_dirLight.color = glm::vec3(1.0f, 0.95f, 0.9f);
    m_dirLight.intensity = 3.0f;

    loadShaders();
    loadAssets();
    setupPhysics();
    setupTestScene();
    setupWeapons();

    m_weaponView.init();
    m_impacts.init();

    // Set hit callback to create impact decals
    m_weapons.setHitCallback([this](const HitResult& hit) {
        m_impacts.addImpact(hit);
    });

    m_running = true;
    LOG_INFO("=== Engine Initialized Successfully ===");
    return true;
}

void Engine::run() {
    LOG_INFO("=== Entering Main Loop ===");
    while (m_running && !m_window.shouldClose()) {
        glfwPollEvents();
        m_timer.tick();
        float dt = m_timer.deltaTime();
        m_window.pollSize();
        processInput();
        update(dt);
        render();
        m_window.swapBuffers();
    }
    LOG_INFO("=== Main Loop Ended ===");
}

void Engine::processInput() {
    m_input.update();

    // ESC: context-dependent exit
    if (m_input.isKeyJustPressed(GLFW_KEY_ESCAPE)) {
        if (m_inVehicle) {
            m_inVehicle = false;
            m_activeVehicle = nullptr;
            m_cursorCaptured = true;
            m_input.setCursorCaptured(true);
        } else if (m_cursorCaptured) {
            m_cursorCaptured = false;
            m_input.setCursorCaptured(false);
        } else {
            m_running = false;
        }
    }
    if (m_input.isKeyJustPressed(GLFW_KEY_TAB)) m_imguiEnabled = !m_imguiEnabled;

    if (m_inVehicle) {
        updateVehicleControls();
    } else {
        // Mouse look
        if (m_cursorCaptured) {
            glm::vec2 delta = m_input.mouseDelta();
            m_camera.look(delta.x, delta.y);

            // Apply weapon recoil to camera
            glm::vec2 recoil = m_weapons.recoilOffset();
            if (recoil.x > 0.01f || std::abs(recoil.y) > 0.01f) {
                m_camera.look(recoil.y * 0.5f, -recoil.x * 0.5f);
            }
        }
        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && !m_cursorCaptured) {
            m_cursorCaptured = true; m_input.setCursorCaptured(true);
        }

        // Camera-relative movement
        float speed = m_input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) ? 2.5f : 1.0f;
        glm::vec3 camFront = m_camera.front();
        glm::vec3 camRight = m_camera.right();
        // Project to XZ plane (no vertical movement from WASD)
        glm::vec3 forward(camFront.x, 0.0f, camFront.z);
        glm::vec3 right(camRight.x, 0.0f, camRight.z);
        float fwdLen = glm::length(forward);
        if (fwdLen > 0.001f) forward /= fwdLen;
        float rightLen = glm::length(right);
        if (rightLen > 0.001f) right /= rightLen;

        glm::vec3 moveDir(0.0f);
        if (m_input.isKeyPressed(GLFW_KEY_W)) moveDir += forward;
        if (m_input.isKeyPressed(GLFW_KEY_S)) moveDir -= forward;
        if (m_input.isKeyPressed(GLFW_KEY_D)) moveDir += right;
        if (m_input.isKeyPressed(GLFW_KEY_A)) moveDir -= right;
        m_player.setMoveInput(moveDir * speed * m_timer.deltaTime());
        if (m_input.isKeyJustPressed(GLFW_KEY_SPACE)) m_player.jump();
        if (m_input.isKeyPressed(GLFW_KEY_LEFT_CONTROL)) m_player.setCrouching(true);
        else m_player.setCrouching(false);

        float scroll = m_input.scrollDelta();
        if (scroll != 0.0f) {
            float fov = std::clamp(m_camera.fov() - scroll * 2.0f, 20.0f, 110.0f);
            m_camera.setFov(fov);
        }

        // ── FPS Controls ──
        // LMB: Fire weapon
        if (m_input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT) && m_cursorCaptured) {
            glm::vec3 origin = m_camera.position();
            glm::vec3 dir = m_camera.front();
            m_weapons.tryFire(origin, dir);
        }

        // RMB: ADS (Aim Down Sights)
        if (m_input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_weapons.startADS();
        } else {
            m_weapons.stopADS();
        }

        // R: Reload
        if (m_input.isKeyJustPressed(GLFW_KEY_R)) {
            m_weapons.tryReload();
        }

        // Q: Switch weapon
        if (m_input.isKeyJustPressed(GLFW_KEY_Q)) {
            m_weapons.switchNext();
        }

        // Number keys: Direct weapon slot selection
        if (m_input.isKeyJustPressed(GLFW_KEY_1)) m_weapons.switchTo(0);
        if (m_input.isKeyJustPressed(GLFW_KEY_2)) m_weapons.switchTo(1);
        if (m_input.isKeyJustPressed(GLFW_KEY_3)) m_weapons.switchTo(2);
        if (m_input.isKeyJustPressed(GLFW_KEY_4)) m_weapons.switchTo(3);
        if (m_input.isKeyJustPressed(GLFW_KEY_5)) m_weapons.switchTo(4);

        // F: Toggle flashlight
        if (m_input.isKeyJustPressed(GLFW_KEY_F)) {
            m_flashlight.toggle();
            LOG_INFO("Flashlight: %s", m_flashlight.isOn() ? "ON" : "OFF");
        }

        // E: Enter/exit vehicle
        if (m_input.isKeyJustPressed(GLFW_KEY_E)) {
            tryEnterVehicle();
        }
    }
}

void Engine::update(float dt) {
    // Update vehicle controls BEFORE physics step
    if (m_inVehicle && m_activeVehicle) {
        m_activeVehicle->update(dt);
    }

    m_physics.step(dt);

    if (m_inVehicle && m_activeVehicle) {
        glm::mat4 chassis = m_activeVehicle->chassisTransform();
        glm::vec3 driverOffset(0.0f, 0.8f, 0.3f);
        glm::vec4 driverPos = chassis * glm::vec4(driverOffset, 1.0f);
        m_camera.setPosition(glm::vec3(driverPos));
    } else {
        m_player.update(dt);
        glm::vec3 playerPos = m_player.position();
        m_camera.setPosition(playerPos + glm::vec3(0, m_player.eyeHeight(), 0));
    }

    m_camera.update(dt);

    // Update game systems
    bool isMoving = false;
    if (!m_inVehicle) {
        glm::vec3 vel = m_player.velocity();
        isMoving = (vel.x * vel.x + vel.z * vel.z) > 0.5f;
    }

    m_weapons.update(dt);
    m_weaponView.update(dt, m_camera, m_weapons, isMoving);
    m_impacts.update(dt);

    // Flashlight follows camera
    m_flashlight.update(m_camera.position(), m_camera.front());
    m_flashlight.updateBattery(dt);
}

void Engine::render() {
    if (m_window.isMinimized()) return;

    renderShadowPass();
    renderScene();
    renderWeaponView();
    renderSkybox();

    if (m_imguiEnabled) renderImGui();
}

void Engine::renderShadowPass() {
    if (!m_shadowShader.id()) return;

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    m_shadowFBO.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);

    m_shadowShader.use();
    glm::mat4 lightSpaceMatrix = calculateLightSpaceMatrix();
    m_shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    m_shadowShader.setMat4("model", glm::mat4(1.0f));
    m_groundPlane.draw();

    for (auto& v : m_vehicles) {
        if (!v->isActive()) continue;
        // Body meshes with chassis transform
        glm::mat4 chassisTF = v->chassisTransform();
        chassisTF = glm::translate(chassisTF, glm::vec3(0.0f, v->modelYOffset(), 0.0f));
        const auto& meshes = v->model().meshes();
        for (int mi = 0; mi < static_cast<int>(meshes.size()); ++mi) {
            int wg = v->findWheelGroup(mi);
            if (wg >= 0 && wg < v->wheelCount()) {
                // Wheel mesh: shift mesh center to wheel origin, then apply wheel world transform
                glm::mat4 wTF = v->wheelTransform(wg);
                wTF = glm::translate(wTF, -v->wheelMeshCenter(wg));
                m_shadowShader.setMat4("model", wTF);
            } else {
                m_shadowShader.setMat4("model", chassisTF);
            }
            meshes[mi].draw();
        }
    }
    glCullFace(GL_BACK);
    m_shadowFBO.unbind();
}

void Engine::renderScene() {
    glViewport(0, 0, m_window.framebufferWidth(), m_window.framebufferHeight());
    glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!m_pbrShader.id()) return;

    m_pbrShader.use();

    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 projection = m_camera.projectionMatrix(m_window.aspectRatio());

    // Apply ADS FOV
    if (m_weapons.adsBlend() > 0.01f && m_weapons.currentWeapon()) {
        float adsFov = m_weapons.currentWeapon()->stats.adsZoomFov;
        float hipFov = m_camera.fov();
        float currentFov = glm::mix(hipFov, adsFov, m_weapons.adsBlend());
        float aspect = m_window.aspectRatio();
        projection = glm::perspective(glm::radians(currentFov), aspect, 0.1f, 1000.0f);
    }

    m_pbrShader.setMat4("view", view);
    m_pbrShader.setMat4("projection", projection);
    m_pbrShader.setVec3("viewPos", m_camera.position());

    // Light
    m_pbrShader.setVec3("lightDir", m_dirLight.direction);
    m_pbrShader.setVec3("lightColor", m_dirLight.color);
    m_pbrShader.setFloat("lightIntensity", m_dirLight.intensity);
    m_pbrShader.setVec3("ambientColor", glm::vec3(0.03f));

    // Flashlight (as spotlight)
    if (m_flashlight.isOn()) {
        m_pbrShader.setVec3("flashPos", m_flashlight.position());
        m_pbrShader.setVec3("flashDir", m_flashlight.direction());
        m_pbrShader.setVec3("flashColor", m_flashlight.color());
        m_pbrShader.setFloat("flashIntensity", m_flashlight.intensity());
        m_pbrShader.setFloat("flashInnerCone", glm::cos(glm::radians(m_flashlight.innerCone())));
        m_pbrShader.setFloat("flashOuterCone", glm::cos(glm::radians(m_flashlight.outerCone())));
        m_pbrShader.setInt("useFlashlight", 1);
    } else {
        m_pbrShader.setInt("useFlashlight", 0);
    }

    // Shadow map
    glm::mat4 lightSpaceMatrix = calculateLightSpaceMatrix();
    m_pbrShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, m_shadowFBO.depthTexture());
    m_pbrShader.setInt("shadowMap", 5);

    // Ground
    m_pbrShader.setMat4("model", glm::mat4(1.0f));
    m_pbrShader.setVec3("albedo", glm::vec3(0.4f));
    m_pbrShader.setFloat("roughness", 0.9f);
    m_pbrShader.setFloat("metallic", 0.0f);
    m_pbrShader.setFloat("ao", 1.0f);
    m_pbrShader.setInt("hasDiffuseMap", 0);
    m_pbrShader.setInt("hasNormalMap", 0);
    m_pbrShader.setInt("hasRoughnessMap", 0);
    m_pbrShader.setInt("hasMetallicMap", 0);
    m_pbrShader.setInt("hasAoMap", 0);
    m_groundPlane.draw();

    // Vehicles (render with model materials)
    for (auto& v : m_vehicles) {
        if (!v->isActive()) continue;
        glm::mat4 chassisTF = v->chassisTransform();
        chassisTF = glm::translate(chassisTF, glm::vec3(0.0f, v->modelYOffset(), 0.0f));
        const auto& meshes = v->model().meshes();
        for (int mi = 0; mi < static_cast<int>(meshes.size()); ++mi) {
            // Set model matrix: chassis for body, physics position for wheels
            int wg = v->findWheelGroup(mi);
            if (wg >= 0 && wg < v->wheelCount()) {
                // Wheel mesh: shift mesh center to wheel origin, then apply wheel world transform
                glm::mat4 wTF = v->wheelTransform(wg);
                wTF = glm::translate(wTF, -v->wheelMeshCenter(wg));
                m_pbrShader.setMat4("model", wTF);
            } else {
                m_pbrShader.setMat4("model", chassisTF);
            }
            const auto& mesh = meshes[mi];
            const auto& mat = mesh.material();
            m_pbrShader.setVec3("albedo", mat.albedo);
            m_pbrShader.setFloat("roughness", mat.roughness);
            m_pbrShader.setFloat("metallic", mat.metallic);
            m_pbrShader.setFloat("ao", mat.ao);
            m_pbrShader.setInt("hasDiffuseMap", mat.diffuseMap ? 1 : 0);
            if (mat.diffuseMap) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mat.diffuseMap); m_pbrShader.setInt("diffuseMap", 0); }
            m_pbrShader.setInt("hasNormalMap", mat.normalMap ? 1 : 0);
            if (mat.normalMap) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, mat.normalMap); m_pbrShader.setInt("normalMap", 1); }
            m_pbrShader.setInt("hasRoughnessMap", mat.roughnessMap ? 1 : 0);
            if (mat.roughnessMap) { glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, mat.roughnessMap); m_pbrShader.setInt("roughnessMap", 2); }
            m_pbrShader.setInt("hasMetallicMap", mat.metallicMap ? 1 : 0);
            if (mat.metallicMap) { glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, mat.metallicMap); m_pbrShader.setInt("metallicMap", 3); }
            m_pbrShader.setInt("hasAoMap", mat.aoMap ? 1 : 0);
            if (mat.aoMap) { glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, mat.aoMap); m_pbrShader.setInt("aoMap", 4); }
            mesh.draw();
        }
    }
}

void Engine::renderWeaponView() {
    if (!m_pbrShader.id() || m_inVehicle) return;

    Model* weaponModel = m_weapons.currentModel();
    if (!weaponModel || weaponModel->meshes().empty()) return;

    m_pbrShader.use();

    // Compute weapon view-model matrix
    glm::mat4 weaponVM = m_weaponView.computeViewModel(m_camera, m_weapons);
    glm::mat4 projection = m_camera.projectionMatrix(m_window.aspectRatio());

    // Use projection as-is but replace view with weapon's view-model
    m_pbrShader.setMat4("view", weaponVM);
    m_pbrShader.setMat4("projection", projection);
    m_pbrShader.setVec3("viewPos", glm::vec3(0.0f));
    m_pbrShader.setMat4("model", glm::mat4(1.0f));

    // Disable shadows for weapon view (looks weird in FP)
    m_pbrShader.setMat4("lightSpaceMatrix", glm::mat4(1.0f));

    // Flashlight off for weapon view
    m_pbrShader.setInt("useFlashlight", 0);

    // Light
    m_pbrShader.setVec3("lightDir", glm::vec3(0.0f, -1.0f, 0.0f));
    m_pbrShader.setVec3("lightColor", glm::vec3(1.0f));
    m_pbrShader.setFloat("lightIntensity", 2.0f);
    m_pbrShader.setVec3("ambientColor", glm::vec3(0.3f));

    // Render weapon meshes
    for (const auto& mesh : weaponModel->meshes()) {
        const auto& mat = mesh.material();
        m_pbrShader.setVec3("albedo", mat.albedo);
        m_pbrShader.setFloat("roughness", mat.roughness);
        m_pbrShader.setFloat("metallic", mat.metallic);
        m_pbrShader.setFloat("ao", mat.ao);
        m_pbrShader.setInt("hasDiffuseMap", mat.diffuseMap ? 1 : 0);
        if (mat.diffuseMap) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, mat.diffuseMap); m_pbrShader.setInt("diffuseMap", 0); }
        m_pbrShader.setInt("hasNormalMap", mat.normalMap ? 1 : 0);
        if (mat.normalMap) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, mat.normalMap); m_pbrShader.setInt("normalMap", 1); }
        m_pbrShader.setInt("hasRoughnessMap", mat.roughnessMap ? 1 : 0);
        if (mat.roughnessMap) { glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, mat.roughnessMap); m_pbrShader.setInt("roughnessMap", 2); }
        m_pbrShader.setInt("hasMetallicMap", mat.metallicMap ? 1 : 0);
        if (mat.metallicMap) { glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, mat.metallicMap); m_pbrShader.setInt("metallicMap", 3); }
        m_pbrShader.setInt("hasAoMap", mat.aoMap ? 1 : 0);
        if (mat.aoMap) { glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, mat.aoMap); m_pbrShader.setInt("aoMap", 4); }
        mesh.draw();
    }
}

void Engine::renderSkybox() {
    if (m_skybox.isLoaded()) {
        m_skybox.exposure = m_exposure;
        m_skybox.draw(m_camera.viewMatrix(), m_camera.projectionMatrix(m_window.aspectRatio()));
    }
}

void Engine::renderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Phantom Strike - Phase 4");
    ImGui::Text("FPS: %.1f (%.2f ms)", m_timer.fps(), 1000.0f / (m_timer.fps() > 0 ? m_timer.fps() : 1.0f));
    ImGui::Separator();

    // Player
    glm::vec3 playerPos = m_player.position();
    ImGui::Text("Player: (%.1f, %.1f, %.1f)", playerPos.x, playerPos.y, playerPos.z);
    ImGui::Text("Grounded: %s | Crouching: %s",
                m_player.isGrounded() ? "Yes" : "No",
                m_player.isCrouching() ? "Yes" : "No");
    ImGui::Separator();

    // Weapon info
    const auto* w = m_weapons.currentWeapon();
    if (w) {
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%s", w->stats.name.c_str());
        if (w->stats.category != WeaponCategory::MELEE) {
            ImGui::Text("Ammo: %d / %d", w->currentMag, w->reserveAmmo);
        }
        ImGui::Text("ADS: %.0f%% | Reloading: %s",
                     m_weapons.adsBlend() * 100.0f,
                     m_weapons.isReloading() ? "Yes" : "No");
        if (m_weapons.isReloading()) {
            ImGui::ProgressBar(m_weapons.reloadProgress());
        }
    } else {
        ImGui::Text("No weapon equipped");
    }
    ImGui::Separator();

    // Flashlight
    ImGui::Text("Flashlight: %s (%.0f%%)", m_flashlight.isOn() ? "ON" : "OFF", m_flashlight.battery());
    ImGui::Separator();

    // Vehicle
    if (m_inVehicle && m_activeVehicle) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "DRIVING: %.0f km/h", m_activeVehicle->speedKmh());
    }
    ImGui::Separator();

    // Controls
    ImGui::Text("WASD Move | Mouse Look | Scroll FOV");
    ImGui::Text("LMB Fire | RMB ADS | R Reload");
    ImGui::Text("Q Switch | 1-5 Select | F Flashlight");
    ImGui::Text("E Enter car | Space Jump | Ctrl Crouch");
    ImGui::End();

    // Crosshair
    if (m_cursorCaptured && !m_inVehicle) {
        ImVec2 center = ImGui::GetIO().DisplaySize;
        center.x *= 0.5f; center.y *= 0.5f;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        float size = 8.0f;
        dl->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y),
                     IM_COL32(255, 255, 255, 200), 1.5f);
        dl->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size),
                     IM_COL32(255, 255, 255, 200), 1.5f);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Engine::loadShaders() {
    if (!m_pbrShader.load("assets/shaders/pbr.vert", "assets/shaders/pbr.frag"))
        LOG_WARN("PBR shader failed to load");
    if (!m_shadowShader.load("assets/shaders/shadow.vert", "assets/shaders/shadow.frag"))
        LOG_WARN("Shadow shader failed to load");
    m_basicShader.load("assets/shaders/basic.vert", "assets/shaders/basic.frag");
}

void Engine::loadAssets() {
    std::string skyboxPath = "assets/textures/skybox/qwantani_dusk_2_puresky_1k.hdr";
    if (!m_skybox.loadHDR(skyboxPath, 512))
        LOG_WARN("Skybox not loaded");
}

void Engine::setupPhysics() {
    m_physics.init(glm::vec3(0.0f, -9.81f, 0.0f));

    auto* groundBody = createGroundPlane(m_physics, 0.0f);
    m_staticBodies.push_back(groundBody);

    m_player.init(m_physics, glm::vec3(0.0f, 1.0f, 3.0f));
    LOG_INFO("Physics setup complete");
}

void Engine::setupTestScene() {
    std::vector<Vertex> vertices = {
        {{-15, 0, -15}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, -1}},
        {{ 15, 0, -15}, {0, 1, 0}, {1, 0}, {1, 0, 0}, {0, 0, -1}},
        {{ 15, 0,  15}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, -1}},
        {{-15, 0, -15}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, -1}},
        {{ 15, 0,  15}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, -1}},
        {{-15, 0,  15}, {0, 1, 0}, {0, 1}, {1, 0, 0}, {0, 0, -1}},
    };
    std::vector<unsigned int> indices = { 0, 1, 2, 3, 4, 5 };
    m_groundPlane.setup(vertices, indices);
    m_groundPlane.setName("GroundPlane");

    auto* w1 = createStaticBox(m_physics, {5.0f, 1.5f, 0.25f}, {0.0f, 1.5f, -8.0f});
    m_staticBodies.push_back(w1);
    auto* w2 = createStaticBox(m_physics, {0.25f, 1.5f, 5.0f}, {-8.0f, 1.5f, 0.0f});
    m_staticBodies.push_back(w2);
    auto* c1 = createStaticBox(m_physics, {0.5f, 0.5f, 0.5f}, {3.0f, 0.5f, -3.0f});
    m_staticBodies.push_back(c1);
    auto* c2 = createStaticBox(m_physics, {0.5f, 0.5f, 0.5f}, {3.0f, 1.5f, -3.0f});
    m_staticBodies.push_back(c2);

    // Vehicle
    auto vehicle = std::make_unique<Vehicle>();
    VehicleConfig sedanConfig = Vehicle::defaultConfig();
    if (vehicle->init(m_physics, "assets/models/cars/2013_lexus_is-f.glb",
                       glm::vec3(5.0f, 0.0f, 0.0f), sedanConfig)) {
        m_vehicles.push_back(std::move(vehicle));
    }
}

void Engine::setupWeapons() {
    m_weapons.init(&m_physics);

    // Add weapons to inventory
    m_weapons.addWeapon(Weapons::ak74m(), "assets/models/weapons/low-poly_ak-74m_zenitco.glb");
    m_weapons.addWeapon(Weapons::pistol9mm(), "assets/models/weapons/9mm_pistol.glb");
    m_weapons.addWeapon(Weapons::shotgun(), "assets/models/weapons/low-poly_benelli_m4.glb");
    m_weapons.addWeapon(Weapons::sniper(), "assets/models/weapons/highly_detailed_sniper_rifle.glb");
    m_weapons.addWeapon(Weapons::meleeBlade(), "assets/models/weapons/melee_weapon_slade_wilson_blade.glb");

    LOG_INFO("Weapons equipped: %d", m_weapons.weaponCount());
}

void Engine::tryEnterVehicle() {
    if (m_inVehicle) return;

    glm::vec3 playerPos = m_player.position();
    float closestDist = 5.0f;
    Vehicle* closest = nullptr;

    for (auto& v : m_vehicles) {
        if (!v->isActive()) continue;
        float dist = glm::distance(playerPos, v->position());
        if (dist < closestDist) {
            closestDist = dist;
            closest = v.get();
        }
    }

    if (closest) {
        m_inVehicle = true;
        m_activeVehicle = closest;
        m_cursorCaptured = true;
        m_input.setCursorCaptured(true);
        m_vehicleSteering = 0.0f;
        m_vehicleThrottle = 0.0f;
        LOG_INFO("Entered vehicle");
    }
}

void Engine::updateVehicleControls() {
    if (!m_activeVehicle) return;

    m_vehicleThrottle = 0.0f;
    if (m_input.isKeyPressed(GLFW_KEY_W)) m_vehicleThrottle = 1.0f;
    if (m_input.isKeyPressed(GLFW_KEY_S)) m_vehicleThrottle = -0.5f;

    float targetSteer = 0.0f;
    if (m_input.isKeyPressed(GLFW_KEY_A)) targetSteer = 1.0f;
    if (m_input.isKeyPressed(GLFW_KEY_D)) targetSteer = -1.0f;
    float steerSpeed = 3.0f;
    m_vehicleSteering += (targetSteer - m_vehicleSteering) * steerSpeed * m_timer.deltaTime();

    float brake = m_input.isKeyPressed(GLFW_KEY_SPACE) ? 1.0f : 0.0f;
    bool handbrake = m_input.isKeyPressed(GLFW_KEY_LEFT_SHIFT);

    if (m_cursorCaptured) {
        glm::vec2 delta = m_input.mouseDelta();
        m_camera.look(delta.x * 0.5f, delta.y * 0.5f);
    }

    m_activeVehicle->setThrottle(m_vehicleThrottle);
    m_activeVehicle->setSteering(m_vehicleSteering);
    m_activeVehicle->setBrake(brake);
    m_activeVehicle->setHandbrake(handbrake);
}

glm::mat4 Engine::calculateLightSpaceMatrix() const {
    float nearPlane = 0.1f, farPlane = 50.0f;
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, nearPlane, farPlane);
    glm::vec3 lightDirNorm = glm::normalize(-m_dirLight.direction);
    glm::mat4 lightView = glm::lookAt(
        glm::vec3(0.0f) + lightDirNorm * 20.0f,
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    return lightProjection * lightView;
}

} // namespace ps
