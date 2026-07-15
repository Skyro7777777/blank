cmake_minimum_required(VERSION 3.25)
project(PhantomStrike VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output all binaries to project root for easy access
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ── Find packages via pkg-config ──
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(ASSIMP REQUIRED assimp)
pkg_check_modules(BULLET REQUIRED bullet)
pkg_check_modules(OPENAL REQUIRED openal)

# GLM is header-only - find via CMake or fallback to UCRT64 path
find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
    # MSYS2 installs GLM headers to ucrt64/include/glm
    set(GLM_INCLUDE_DIR "${CMAKE_SYSTEM_PREFIX_PATH}/include")
    if(NOT EXISTS "${GLM_INCLUDE_DIR}/glm/glm.hpp")
        # Try common MSYS2 path
        set(GLM_INCLUDE_DIR "C:/msys64/ucrt64/include")
    endif()
    message(STATUS "GLM include dir: ${GLM_INCLUDE_DIR}")
endif()

# ── GLAD (generated source) ──
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/src/vendor/glad)
set(GLAD_SOURCES
    ${GLAD_DIR}/src/gl.c
)

# ── stb (header-only) ──
set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/vendor/stb)

# ── Dear ImGui ──
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/vendor/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

# ── Project sources ──
set(PROJECT_SOURCES
    src/main.cpp
    # Core
    src/core/Engine.cpp
    src/core/Window.cpp
    src/core/Input.cpp
    src/core/Timer.cpp
    src/core/Logger.cpp
    # Renderer
    src/renderer/Shader.cpp
    src/renderer/Camera.cpp
    src/renderer/Texture.cpp
    src/renderer/Mesh.cpp
    src/renderer/Model.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Skybox.cpp
    # Physics
    src/physics/PhysicsWorld.cpp
    src/physics/CollisionShapes.cpp
    src/physics/PlayerController.cpp
    src/physics/Vehicle.cpp
    # Game
    src/game/Weapon.cpp
    src/game/WeaponSystem.cpp
    src/game/WeaponView.cpp
    src/game/ImpactSystem.cpp
    src/game/Flashlight.cpp
)

# ── Executable ──
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${GLAD_SOURCES} ${IMGUI_SOURCES})

# ── Include directories ──
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GLAD_DIR}/include
    ${STB_INCLUDE_DIR}
    ${IMGUI_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
)

# GLM include (header-only)
if(glm_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
else()
    target_include_directories(${PROJECT_NAME} PRIVATE ${GLM_INCLUDE_DIR})
endif()

# ── Link libraries ──
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${GLFW_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    ${BULLET_LIBRARIES}
    ${OPENAL_LIBRARIES}
    opengl32
)

# ── Link directories ──
target_link_directories(${PROJECT_NAME} PRIVATE
    ${GLFW_LIBRARY_DIRS}
    ${ASSIMP_LIBRARY_DIRS}
    ${BULLET_LIBRARY_DIRS}
    ${OPENAL_LIBRARY_DIRS}
)

# ── Compile definitions ──
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ${GLFW_CFLAGS_OTHER}
    ${ASSIMP_CFLAGS_OTHER}
)

# ── Windows-specific ──
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# ── Copy assets to build directory ──
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory..."
)
