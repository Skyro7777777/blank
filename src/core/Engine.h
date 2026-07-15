#pragma once

// GLAD must be included before GLFW
#include <glad/gl.h>

#include "core/Window.h"
#include "core/Input.h"
#include "core/Timer.h"
#include "renderer/Camera.h"
#include "renderer/Shader.h"
#include "renderer/Model.h"
#include "renderer/Skybox.h"
#include "renderer/Framebuffer.h"
#include "renderer/Light.h"
#include "renderer/Mesh.h"
#include "physics/PhysicsWorld.h"
#include "physics/CollisionShapes.h"
#include "physics/PlayerController.h"
#include "physics/Vehicle.h"
#include "game/WeaponSystem.h"
#include "game/WeaponView.h"
#include "game/ImpactSystem.h"
#include "game/Flashlight.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <vector>
#include <memory>

namespace ps {

class Engine {
public:
    Engine();
    ~Engine();

    bool init(const Window::Config& config = {});
    void run();
    void quit() { m_running = false; }

    Window& window() { return m_window; }
    Input& input() { return m_input; }
    Timer& timer() { return m_timer; }
    Camera& camera() { return m_camera; }
    PhysicsWorld& physics() { return m_physics; }

    bool isRunning() const { return m_running; }

private:
    void processInput();
    void update(float dt);
    void render();
    void renderShadowPass();
    void renderScene();
    void renderSkybox();
    void renderWeaponView();
    void renderImGui();

    void loadShaders();
    void loadAssets();
    void setupPhysics();
    void setupTestScene();
    void setupWeapons();
    glm::mat4 calculateLightSpaceMatrix() const;

    // Player-vehicle interaction
    void tryEnterVehicle();
    void updateVehicleControls();

    // Subsystems
    Window m_window;
    Input m_input;
    Timer m_timer;
    Camera m_camera;

    // Physics
    PhysicsWorld m_physics;
    PlayerController m_player;
    std::vector<std::unique_ptr<Vehicle>> m_vehicles;
    Vehicle* m_activeVehicle = nullptr;

    // Game systems
    WeaponSystem m_weapons;
    WeaponView m_weaponView;
    ImpactSystem m_impacts;
    Flashlight m_flashlight;

    // Collision shapes/bodies owned by engine
    std::vector<btCollisionShape*> m_ownedShapes;
    std::vector<btRigidBody*> m_staticBodies;

    // Shaders
    Shader m_pbrShader;
    Shader m_shadowShader;
    Shader m_basicShader;

    // Assets
    Model m_testModel;
    Mesh m_groundPlane;
    Skybox m_skybox;

    // Shadow mapping
    Framebuffer m_shadowFBO;
    static constexpr int SHADOW_WIDTH = 2048;
    static constexpr int SHADOW_HEIGHT = 2048;

    // Lighting
    DirectionalLight m_dirLight;

    // State
    bool m_running = false;
    bool m_imguiEnabled = true;
    bool m_cursorCaptured = true;
    bool m_showShadows = true;
    bool m_inVehicle = false;
    float m_exposure = 1.0f;

    // Vehicle control smoothing
    float m_vehicleSteering = 0.0f;
    float m_vehicleThrottle = 0.0f;

    const char* m_glslVersion = "#version 330 core";
};

} // namespace ps
