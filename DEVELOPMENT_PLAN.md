# PHANTOM STRIKE - 3D Realistic Shooting Game
## Development Plan (Windows | C++ | MSYS2/MinGW-w64)

---

## 1. PROJECT OVERVIEW

**Game Name**: PHANTOM STRIKE  
**Genre**: First-Person Shooter (FPS)  
**Platform**: Windows (x64)  
**Primary Language**: C++ (C++20)  
**Target**: Realistic tactical military shooter with modern rendering

---

## 2. ENGINE & TECHNOLOGY STACK

### RECOMMENDED APPROACH: Custom C++ Engine with Modern OpenGL

Why NOT use an existing engine like Unreal/Godot?
- Full control over rendering pipeline for maximum realism
- Learning experience and optimization control
- No licensing fees or restrictions
- Can be tailored specifically for FPS mechanics

### Toolchain: MSYS2 + MinGW-w64 (NO Visual Studio)

**Why MSYS2 instead of Visual Studio 2022?**
- VS2022 = ~8 GB install | MSYS2 = ~500 MB install
- Same compiler quality (GCC 14.x / Clang 18.x)
- pacman package manager (like Arch Linux) - faster than vcpkg
- Native Windows binaries (no Cygwin DLL dependency)
- UCRT64 environment = modern Windows C runtime
- All game dev libraries available as pre-built packages
- Unix-like build tools (make, pkg-config) work natively

### Core Libraries

| Component | Library | MSYS2 Package | Purpose |
|-----------|---------|---------------|---------|
| **Compiler** | GCC 14.x | `mingw-w64-ucrt-x86_64-gcc` | C++20 compiler |
| **Build** | CMake 3.x | `mingw-w64-ucrt-x86_64-cmake` | Build system |
| **Make** | Ninja | `mingw-w64-ucrt-x86_64-ninja` | Fast build runner |
| **Windowing** | GLFW 3.4 | `mingw-w64-ucrt-x86_64-glfw` | Window, input, OpenGL context |
| **OpenGL Loader** | GLAD (GL 4.6) | *Included as source* | Modern OpenGL function loading |
| **Math** | GLM 1.0.3 | `mingw-w64-ucrt-x86_64-glm` | Vectors, matrices, quaternions |
| **Model Loading** | Assimp 6.0.5 | `mingw-w64-ucrt-x86_64-assimp` | Load FBX/OBJ/glTF 3D models |
| **Physics** | Bullet 3.25 | `mingw-w64-ucrt-x86_64-bullet` | Rigid body, collision, raycasting |
| **Audio** | OpenAL 1.25 | `mingw-w64-ucrt-x86_64-openal` | 3D positional audio, HRTF |
| **Image Loading** | stb_image | *Included as source* | Texture loading (PNG, JPG, HDR) |
| **UI** | Dear ImGui | *Included as source* | Debug UI, HUD overlay |
| **JSON** | nlohmann-json | `mingw-w64-ucrt-x86_64-nlohmann-json` | Level/config file parsing |
| **Pkg Config** | pkg-config | `mingw-w64-ucrt-x86_64-pkg-config` | Find library flags automatically |

### Included-as-Source (Not in pacman)
These are single-header or small libraries that we drop into `src/vendor/`:
- **GLAD**: Generate from https://glad.dav1d.de/ (OpenGL 4.6 Core, C/C++)
- **stb_image.h**: From https://github.com/nothings/stb
- **Dear ImGui**: From https://github.com/ocornut/imgui (backends: glfw + opengl3)
- **miniaudio**: From https://github.com/mackron/miniaudio (audio fallback)

### Build Tools
- **Compiler**: GCC 14.x (MinGW-w64 UCRT64) — C++20
- **Build System**: CMake + Ninja
- **Package Manager**: pacman (MSYS2 built-in)
- **Debugger**: GDB (comes with GCC)

---

## 3. RENDERING PIPELINE (For Realism)

### Phase 1: Forward Rendering (MVP)
- Basic PBR shading (Cook-Torrance BRDF)
- Directional light + point lights
- Shadow mapping
- Skybox

### Phase 2: Deferred Rendering
- G-Buffer (Position, Normal, Albedo, PBR parameters)
- Deferred PBR shading
- Image-Based Lighting (IBL)
- HDR tone mapping + Bloom

### Phase 3: Advanced Lighting
- Cascaded Shadow Maps (CSM)
- Screen Space Ambient Occlusion (SSAO)
- Volumetric lighting / God rays
- Spot lights, area lights

### Phase 4: Post-Processing
- Temporal Anti-Aliasing (TAA)
- Motion blur
- Depth of field
- Chromatic aberration
- Film grain
- Color grading

### Phase 5: Advanced Features
- Screen Space Reflections (SSR)
- Parallax mapping
- Particle systems (smoke, muzzle flash, blood, debris)
- **Volumetric realistic moving clouds** (animated skybox clouds using noise-based raymarching)
- **Dynamic day/night cycle** with skybox blending (dusk → day → night)
- **Flashlight/torch system** (spotlight cone, volumetric light, battery drain)
- **Night gameplay** with limited visibility, torch as primary light source

---

## 4. PROJECT STRUCTURE

```
PhantomStrike/
├── CMakeLists.txt
├── assets/
│   ├── models/
│   │   ├── weapons/              # FPS weapon models (GLB)
│   │   │   ├── 9mm_pistol.glb
│   │   │   ├── heavy_revolver.glb
│   │   │   ├── low-poly_ak-74m_zenitco.glb
│   │   │   ├── low-poly_benelli_m4.glb
│   │   │   ├── low-poly_scifi_katana.glb
│   │   │   ├── m82.glb
│   │   │   ├── sniper_rifle.glb
│   │   │   ├── ump40.glb
│   │   │   └── ... (more weapons)
│   │   ├── characters/           # Enemy/soldier models (GLB)
│   │   │   ├── guard.glb
│   │   │   ├── soldier.glb
│   │   │   └── us.glb
│   │   ├── cars/                 # Driveable vehicles (GLB)
│   │   │   ├── 2013_lexus_is-f.glb
│   │   │   ├── 2024_byd_m6.glb
│   │   │   ├── 2024_byd_tang.glb
│   │   │   └── 2024_gmc_hummer_ev_suv.glb
│   │   ├── environment/          # Buildings, props, terrain
│   │   │   ├── buildings/
│   │   │   ├── crates/
│   │   │   ├── props/            # Compass, hammer, etc.
│   │   │   │   ├── seadogs_compass/
│   │   │   │   └── cross_pein_hammer/
│   │   │   └── vehicles/
│   │   └── vegetation/           # Trees, grass, ferns, rocks
│   ├── textures/
│   │   ├── pbr/                  # PBR texture sets (1K resolution)
│   │   │   ├── concrete/
│   │   │   ├── metal/
│   │   │   ├── wood/
│   │   │   ├── fabric/
│   │   │   └── ground/
│   │   ├── skybox/               # HDR skybox cubemaps
│   │   └── ui/                   # Crosshair, HUD elements
│   ├── audio/
│   │   ├── weapons/              # Gunshots, reloads, dry-fire
│   │   ├── footsteps/            # Different surfaces
│   │   ├── ambient/              # Environment sounds
│   │   ├── music/                # Background music
│   │   └── foley/                # Body movement, gear
│   ├── shaders/
│   │   ├── pbr.vert / pbr.frag
│   │   ├── shadow.vert / shadow.frag
│   │   ├── deferred.vert / deferred.frag
│   │   ├── ssao.vert / ssao.frag
│   │   ├── bloom.vert / bloom.frag
│   │   ├── particle.vert / particle.frag
│   │   ├── skybox.vert / skybox.frag
│   │   ├── clouds.vert / clouds.frag     # Volumetric moving clouds
│   │   ├── flashlight.vert / flashlight.frag  # Torch/flashlight beam
│   │   ├── vehicle.vert / vehicle.frag   # Car rendering
│   │   └── postprocess.vert / postprocess.frag
│   └── maps/
│       └── training_ground.json  # Level data
├── src/
│   ├── main.cpp                  # Entry point
│   ├── vendor/                   # Included-as-source libraries
│   │   ├── glad/                 # GLAD OpenGL 4.6 loader
│   │   ├── stb/                  # stb_image.h
│   │   └── imgui/                # Dear ImGui source
│   ├── core/
│   │   ├── Engine.h/cpp          # Main engine loop
│   │   ├── Window.h/cpp          # GLFW window wrapper
│   │   ├── Input.h/cpp           # Input system (keyboard, mouse)
│   │   ├── Timer.h/cpp           # Delta time, FPS counter
│   │   └── Logger.h/cpp          # Logging system
│   ├── renderer/
│   │   ├── Renderer.h/cpp        # Main renderer
│   │   ├── Shader.h/cpp          # Shader compilation/linking
│   │   ├── Camera.h/cpp          # FPS camera (pitch/yaw)
│   │   ├── Model.h/cpp           # 3D model loading (Assimp)
│   │   ├── Mesh.h/cpp            # Mesh data
│   │   ├── Texture.h/cpp         # Texture loading
│   │   ├── Framebuffer.h/cpp     # FBO for deferred rendering
│   │   ├── GBuffer.h/cpp         # Geometry buffer
│   │   ├── Light.h/cpp           # Light types
│   │   ├── Skybox.h/cpp          # Skybox rendering
│   │   ├── ParticleSystem.h/cpp  # GPU particle system
│   │   └── PostProcessor.h/cpp   # Post-processing chain
│   ├── physics/
│   │   ├── PhysicsWorld.h/cpp    # Bullet physics wrapper
│   │   ├── CollisionShape.h/cpp  # Shape definitions
│   │   └── Raycast.h/cpp         # Raycasting for shooting
│   ├── audio/
│   │   ├── AudioManager.h/cpp    # OpenAL wrapper
│   │   └── Sound.h/cpp           # Sound loading/playback
│   ├── game/
│   │   ├── Game.h/cpp            # Game state machine
│   │   ├── Player.h/cpp          # Player controller (FPS)
│   │   ├── Weapon.h/cpp          # Weapon system
│   │   ├── Projectile.h/cpp      # Bullet physics
│   │   ├── Enemy.h/cpp           # AI enemy
│   │   ├── HealthSystem.h/cpp    # Damage/health
│   │   ├── Level.h/cpp           # Level loading
│   │   ├── HUD.h/cpp             # Heads-up display
│   │   ├── Car.h/cpp             # Vehicle controller (driveable cars)
│   │   ├── Flashlight.h/cpp      # Torch/flashlight system
│   │   ├── VoiceSystem.h/cpp     # Piper TTS voice command system
│   │   └── DayNightCycle.h/cpp   # Dynamic day/night + skybox blending
│   ├── ai/
│   │   ├── Pathfinding.h/cpp     # A* pathfinding
│   │   ├── BehaviorTree.h/cpp    # Enemy behavior
│   │   └── NavMesh.h/cpp         # Navigation mesh
│   └── utils/
│       ├── Math.h                # Math utilities
│       ├── FileIO.h/cpp          # File reading
│       └── Profiler.h/cpp        # Performance profiler
└── scripts/
    ├── build.sh                  # Build script (MSYS2)
    └── download_assets.sh        # Asset download script
```

---

## 5. DEVELOPMENT PHASES

### PHASE 0: Project Setup (Day 1)
- [ ] Install MSYS2 (ucrt64 environment)
- [ ] Install GCC, CMake, Ninja via pacman
- [ ] Install all dependencies via pacman
- [ ] Generate GLAD OpenGL 4.6 loader
- [ ] Download stb_image.h and ImGui source
- [ ] Set up CMake project structure
- [ ] Create main.cpp with GLFW window + OpenGL context
- [ ] Verify build compiles and runs

### PHASE 1: Core Engine (Week 1)
- [ ] Window management (GLFW wrapper)
- [ ] Input system (mouse + keyboard, raw mouse input)
- [ ] OpenGL 4.6 rendering context
- [ ] Shader loading/compilation system
- [ ] FPS camera controller (look around + WASD movement)
- [ ] Delta time / game loop
- [ ] Logging system

### PHASE 2: Rendering Foundation (Week 2)
- [ ] Model loading (Assimp → our Mesh format)
- [ ] PBR material system (Albedo, Normal, Roughness, Metallic, AO maps)
- [ ] Forward rendering pipeline (start simple, upgrade to deferred)
- [ ] Skybox rendering (HDR cubemap)
- [ ] **4 Skybox HDRIs**: Qwantani Dusk, Qwantani Sunset, Qwantani Moon, NightSkyHDRI013
- [ ] **Day/night cycle**: Smooth skybox blending based on time of day
- [ ] Basic lighting (directional + point lights)
- [ ] Shadow mapping
- [ ] **Flashlight/torch**: Spot light cone with volumetric beam
- [ ] Load first test scene

### PHASE 3: Physics & Interaction (Week 3)
- [ ] Bullet Physics integration
- [ ] Static collision meshes for environment
- [ ] Raycasting system (for shooting)
- [ ] Player physics (capsule collider, gravity, crouch, jump)
- [ ] Object interaction (doors, pickups)
- [ ] **Vehicle physics** (Bullet btRaycastVehicle for driveable cars)
- [ ] Car enter/exit system (press F near car)
- [ ] Vehicle suspension, steering, engine, braking
- [ ] Load car models from `assets/models/cars/`

### PHASE 4: FPS Mechanics (Week 4)
- [ ] Weapon system framework
- [ ] First weapon: Assault Rifle (AK-74M from assets)
- [ ] ADS (Aim Down Sights) mechanic
- [ ] Recoil system (pattern + randomness)
- [ ] Reload mechanic (animation + timer)
- [ ] Muzzle flash (particle + light)
- [ ] Bullet impact effects (sparks, decals)
- [ ] Weapon sway and bobbing
- [ ] Second weapon: Pistol (9mm from assets)
- [ ] Weapon switching
- [ ] **Flashlight/torch** (F key toggle, spotlight cone, volumetric beam)
- [ ] **Melee weapons** (hammer, bat, knife from assets)
- [ ] Load all weapon models from `assets/models/weapons/`

### PHASE 5: Audio System (Week 5)
- [ ] OpenAL initialization
- [ ] 3D positional audio
- [ ] Weapon sounds (gunshots, reloads, dry-fire)
- [ ] Footstep sounds (surface-dependent)
- [ ] Ambient environment audio
- [ ] Impact sounds (bullet hit wall, metal, flesh)
- [ ] HRTF support for realistic 3D audio
- [ ] **Piper TTS voice system integration**
  - [ ] Subprocess call to piper.exe for WAV generation
  - [ ] Voice line caching system (pre-generate on load)
  - [ ] Officer command voice lines
  - [ ] Player callout voice lines
  - [ ] Enemy alert/taunt voice lines
- [ ] Car engine sounds (looping, pitch varies with speed)
- [ ] Flashlight click on/off sound

### PHASE 6: Enemy AI (Week 6)
- [ ] Navigation mesh generation
- [ ] A* pathfinding
- [ ] Basic enemy behavior (patrol, chase, attack)
- [ ] Behavior tree system
- [ ] Enemy model + animations
- [ ] Cover system
- [ ] Enemy health + death

### PHASE 7: Deferred Rendering Upgrade (Week 7)
- [ ] G-Buffer implementation
- [ ] Deferred shading pass
- [ ] SSAO
- [ ] Bloom
- [ ] HDR tone mapping
- [ ] Cascaded shadow maps
- [ ] Post-processing pipeline
- [ ] **Volumetric moving clouds** (raymarching with animated 3D noise)
- [ ] **Night rendering** (enhanced darkness, flashlight volumetrics, muzzle flash bloom)

### PHASE 8: Level Design (Week 8)
- [ ] Level data format (JSON)
- [ ] Training ground level (outdoor)
- [ ] Urban combat level (indoor/outdoor)
- [ ] Level loading system
- [ ] Spawn points and triggers
- [ ] Cover placement

### PHASE 9: Polish & Effects (Week 9-10)
- [ ] Particle system (smoke, dust, sparks, blood)
- [ ] Screen shake on explosions
- [ ] Damage indicators
- [ ] HUD (health, ammo, minimap, crosshair)
- [ ] Kill feed
- [ ] Menu system (main menu, pause, settings)
- [ ] Volumetric fog/god rays
- [ ] Motion blur

### PHASE 10: Final Polish (Week 11-12)
- [ ] Performance optimization
- [ ] LOD system
- [ ] Frustum culling
- [ ] Occlusion culling
- [ ] Asset packing
- [ ] Bug fixing
- [ ] Playtesting
- [ ] Final build

---

## 6. FREE ASSET SOURCES

### 3D Models (Weapons, Characters, Environment)
| Source | URL | What to Get |
|--------|-----|-------------|
| **Poly Haven** | polyhaven.com | CC0 models (props, vehicles, vegetation) |
| **Sketchfab** | sketchfab.com/features/free-3d-models | Free CC-licensed weapons, military models |
| **Free3D** | free3d.com | AK47, M4, pistols, vehicles, buildings (FBX/OBJ) |
| **OpenGameArt** | opengameart.org | Game-ready models, CC0/BY licensed |
| **Quaternius** | quaternius.com | Free low-poly/PBR models |

### PBR Textures (Realistic Materials)
| Source | URL | What to Get |
|--------|-----|-------------|
| **AmbientCG** | ambientcg.com | CC0 PBR textures (all categories) |
| **Poly Haven** | polyhaven.com/textures | CC0 HDRIs, textures, models |
| **FreePBR** | freepbr.com | 600+ PBR texture sets |

### HDR Skyboxes
| Source | URL |
|--------|-----|
| **Poly Haven** | polyhaven.com/hdris |

### Sound Effects
| Source | URL | What to Get |
|--------|-----|-------------|
| **Freesound** | freesound.org | 722K+ CC-licensed sounds |
| **OpenGameArt** | opengameart.org | Game sound effects |

### Specific Asset List (All assets at 1K resolution)

#### USER-CONFIRMED ASSETS (Downloaded / To download)
| # | Type | Asset | Source | Status |
|---|------|-------|--------|--------|
| 1 | Ground Texture | Ground037 | AmbientCG | Pending |
| 2 | Grass Model | Grass Medium 01 | Poly Haven | Pending |
| 3 | Stones Model | Rocky Trail | Poly Haven | Pending |
| 4 | Fern Model | Fern 02 | Poly Haven | Pending |
| 5 | Rocks Model | Rock Moss Set 02 | Poly Haven | Pending |
| 6 | Tree Model | Pine Sapling Small | Poly Haven | Pending |
| 7 | Tree Model | Fir Sapling Medium | Poly Haven | Pending |
| 8 | Street Prop | Street Lamp 01 | Poly Haven | Pending |
| 9 | Prop | Old Tyre | Poly Haven | Pending |
| 10 | Weapon Prop | Baseball Bat | Poly Haven | Pending |
| 11 | Skybox (Dusk) | Qwantani Dusk 2 PureSky | Poly Haven | **Downloaded** (1K HDR) |
| 12 | Skybox (Day/Sunset) | Qwantani Sunset PureSky | Poly Haven | **Downloaded** (1K HDR) |
| 13 | Skybox (Night) | Qwantani Moon Noon PureSky | Poly Haven | **Downloaded** (1K HDR) |
| 14 | Skybox (Night Alt) | NightSkyHDRI013 | AmbientCG | Pending (manual download) |
| 15 | Compass Prop | Seadogs Compass | Poly Haven | Pending |
| 16 | Hammer Melee | Cross Pein Hammer | Poly Haven | Pending |

#### USER-ADDED ASSETS (Already in project)
| Category | File | Size | Format | Notes |
|----------|------|------|--------|-------|
| **Cars** | 2013_lexus_is-f.glb | 3.5 MB | GLB | Needs car physics/animation |
| **Cars** | 2024_byd_m6.glb | 20.5 MB | GLB | Needs car physics/animation |
| **Cars** | 2024_byd_tang.glb | 23.8 MB | GLB | Needs car physics/animation |
| **Cars** | 2024_gmc_hummer_ev_suv.glb | 41.0 MB | GLB | **Large** - may need LOD |
| **Weapons** | 9mm_pistol.glb | 4.4 MB | GLB | FPS weapon |
| **Weapons** | heavy_revolver.glb | 10.9 MB | GLB | FPS weapon |
| **Weapons** | highly_detailed_sniper_rifle.glb | 1.9 MB | GLB | FPS weapon |
| **Weapons** | low-poly_ak-74m_zenitco.glb | 2.8 MB | GLB | FPS weapon |
| **Weapons** | low-poly_benelli_m4.glb | 0.8 MB | GLB | Shotgun |
| **Weapons** | low-poly_scifi_katana.glb | 3.3 MB | GLB | Melee |
| **Weapons** | low-poly_vepr_12_molot.glb | 1.7 MB | GLB | Shotgun variant |
| **Weapons** | low-poly_vhs2_ct.glb | 2.3 MB | GLB | Assault rifle |
| **Weapons** | m82.glb | 15.3 MB | GLB | **Large** - may need LOD |
| **Weapons** | melee_weapon_slade_wilson_blade.glb | 3.1 MB | GLB | Melee |
| **Weapons** | sniper_rifle.glb | 7.2 MB | GLB | Sniper |
| **Weapons** | survival_melee_weapon_1.glb | 5.1 MB | GLB | Melee |
| **Weapons** | ump40.glb | 9.9 MB | GLB | SMG |
| **Characters** | guard.glb | 39.4 MB | GLB | **Large** - may need LOD |
| **Characters** | soldier.glb | 8.2 MB | GLB | Enemy/NPC |
| **Characters** | us.glb | 37.4 MB | GLB | **Large** - may need LOD |

**GLB Format Assessment**: All user-added models are in GLB (binary glTF) format.
- Assimp can load GLB directly — no conversion strictly needed
- Large files (Hummer 41MB, guard 39MB, us 37MB, M82 15MB) should have LODs generated for real-time performance
- LOD generation can be done programmatically (mesh decimation) or via Blender
- PBR textures embedded in GLB will be extracted and optimized during loading

#### PIPER TTS VOICE SYSTEM (Installed)
- **Location**: `E:\SteamLibrary\IInstalled_Programs\PiperTTS\`
- **Executable**: `E:\SteamLibrary\IInstalled_Programs\PiperTTS\piper\piper\piper.exe`
- **Voice Models**: `E:\SteamLibrary\IInstalled_Programs\PiperTTS\voices\`
  - `en_US-lessac-medium.onnx` (61 MB) - Female, clear voice
  - `en_US-libritts_r-medium.onnx` (75 MB) - Male, varied voices
- **Usage**: `echo "Text" | piper.exe -m model.onnx -f output.wav`
- **Integration**: Will be called from game to generate voice lines at runtime
  - Officer commands: "Move out!", "Enemy spotted!", "Take cover!"
  - Player callouts: "Reloading!", "Need backup!", "Contact front!"
  - Pre-generate common phrases at game start, cache as WAV files
  - Runtime generation for dynamic dialogue

#### AUTO-DOWNLOAD ASSETS (I will find and download these)
| Category | Items Needed | Source Priority |
|----------|-------------|----------------|
| **Guns/Weapons** | M4A1, Glock 17, AK47, Knife, Grenades, Shotgun, Sniper Rifle | Poly Haven, Sketchfab (free), OpenGameArt |
| **Vehicles** | Military jeep, truck, Humvee, destroyed car | Poly Haven, Free3D, OpenGameArt |
| **Characters** | Soldier (rigged), enemy variants | Sketchfab (free), Mixamo |
| **Buildings** | Military building, warehouse, walls, sandbags | Poly Haven, OpenGameArt |
| **Props** | Crates, barrels, ammo boxes, sandbags, barricades | Poly Haven, OpenGameArt |
| **PBR Textures** | Concrete, metal, wood, sand, fabric, dirt, brick, plaster | AmbientCG, FreePBR |
| **Skyboxes** | Desert, military base, sunset HDRI | Poly Haven |
| **Audio - Weapons** | Gunshots (rifle, pistol, shotgun), reload, dry-fire, casing drop | Freesound, OpenGameArt |
| **Audio - Footsteps** | Concrete, dirt, grass, metal, wood | Freesound |
| **Audio - Ambient** | Outdoor wind, birds, distant combat | Freesound, OpenGameArt |
| **Audio - Impacts** | Bullet hit wall, metal, flesh, glass, wood | Freesound |
| **Audio - Foley** | Gear rattle, breathing, sprint | Freesound |
| **Audio - Voice** | Officer commands, player callouts (generated via Piper TTS) | Piper TTS |
| **Melee Weapons** | Hammer, bat, knife, katana | Poly Haven |

---

## 7. KEY TECHNICAL DECISIONS

### Rendering: Deferred vs Forward
- **Start with Forward** for simplicity during early development
- **Upgrade to Deferred** in Phase 7 for many lights + PBR
- Final pipeline: Deferred + forward pass for transparency

### Physics: Bullet Physics
- Industry-proven, open source
- Excellent raycasting for hit-scan weapons
- Capsule collider for player
- Triangle mesh colliders for environment
- Rigid bodies for destructible objects
- **Vehicle physics**: Raycast vehicle (Bullet btRaycastVehicle) for driveable cars
  - Suspension, steering, engine force, braking
  - Car models already in `assets/models/cars/` (4 vehicles)
  - Enter/exit vehicle system
  - Vehicle damage model

### Audio: OpenAL Soft
- 3D positional audio with distance attenuation
- HRTF (Head-Related Transfer Function) for realistic 3D sound
- Effects: reverb zones, occlusion

### Voice System: Piper TTS (Local Neural TTS)
- **Installed at**: `E:\SteamLibrary\IInstalled_Programs\PiperTTS\`
- **Engine**: ONNX Runtime based, runs fully offline, no internet needed
- **Latency**: ~0.23s inference for typical voice line
- **Voice models**: en_US-lessac-medium (female), en_US-libritts_r-medium (male)
- **Integration approach**:
  1. Pre-generate common voice lines at game load (cache as WAV)
  2. Runtime generation for dynamic content
  3. Officer commands, player callouts, enemy taunts
  4. Piper called via subprocess, WAV loaded into OpenAL buffer
- **Voice lines to generate**:
  - Officer: "Move out!", "Enemy spotted!", "Take cover!", "Regroup!", "Mission update"
  - Player: "Reloading!", "Need backup!", "Contact front!", "I'm hit!"
  - Enemy: taunts, alerts, death sounds

### Toolchain: MSYS2 UCRT64
- UCRT64 = uses Windows Universal C Runtime (modern, no MSVC needed)
- GCC produces native Windows executables (no DLL dependencies)
- pacman handles all library installs and updates
- Ninja for fast parallel builds

### Vehicles: Bullet RaycastVehicle
- Cars already in `assets/models/cars/` (4 GLB models)
- Bullet's btRaycastVehicle for realistic car physics
- Suspension, steering, engine force, braking
- Enter/exit system (player capsule ↔ vehicle)
- Third-person camera when driving, FPS when on foot

### Night Gameplay & Flashlight
- Night skyboxes: Qwantani Moon, NightSkyHDRI013
- Dynamic flashlight (spotlight cone + volumetric beam)
- Battery drain mechanic (adds tension)
- Torch affects enemy AI detection
- Enhanced night rendering (darker shadows, bloom on lights)

### Volumetric Clouds
- Raymarching-based cloud rendering in fragment shader
- Animated using 3D Perlin/Worley noise over time
- Clouds move with wind direction
- Respond to sun position (scattering, silver lining)
- Integrated with day/night cycle

### Day/Night Cycle
- 4 skybox HDRIs blended based on time of day
- Qwantani Dusk → Qwantani Sunset → Qwantani Moon → NightSkyHDRI013
- Directional light color/intensity changes with time
- Shadow direction and length changes
- Ambient light color shifts (warm day → cool night)

### Coordinate System
- Right-handed: +X right, +Y up, -Z forward (OpenGL convention)
- Units: 1 unit = 1 meter

### Target Performance
- 1080p @ 60 FPS on mid-range hardware
- 1440p @ 60 FPS on high-end hardware
- Draw calls: < 2000 per frame
- Triangle count: < 2M visible per frame

---

## 8. REALISM FEATURES CHECKLIST

### Visual Realism
- [x] PBR materials with proper roughness/metallic
- [x] HDR rendering with bloom
- [x] Cascaded shadow maps
- [x] SSAO for ambient occlusion
- [x] Screen-space reflections
- [x] Motion blur
- [x] Depth of field
- [x] Volumetric fog
- [x] Particle effects (smoke, sparks, dust)
- [x] Bullet impact decals
- [x] Weapon sway and bob
- [x] Muzzle flash with dynamic light
- [x] Volumetric moving clouds (animated raymarching)
- [x] Dynamic day/night cycle with skybox blending
- [x] Flashlight/torch with volumetric beam
- [x] Vehicle rendering with proper materials

### Gameplay Realism
- [x] Realistic weapon handling (ADS, recoil patterns)
- [x] Bullet drop and travel time (optional)
- [x] Magazine-based ammo system
- [x] Reload animations with timer
- [x] Surface-dependent footstep sounds
- [x] 3D positional audio with HRTF
- [x] Damage model (headshot, body, limbs)
- [x] Cover system for AI
- [x] Stamina system (sprint duration)
- [x] Weapon weight affects movement speed
- [x] Driveable vehicles with realistic physics
- [x] Flashlight with battery drain
- [x] Voice commands via Piper TTS (officer, player, enemy)
- [x] Night gameplay with limited visibility

### Physics Realism
- [x] Rigid body physics for objects
- [x] Ragdoll physics for enemies
- [x] Destructible environment elements
- [x] Bullet penetration through thin materials
- [x] Gravity and projectile arc for thrown items
- [x] Vehicle suspension, steering, and collision
- [x] Car damage model

---

## 9. PHASE 0 SETUP INSTRUCTIONS (MSYS2)

### Step 1: Install MSYS2
```powershell
# Download from https://www.msys2.org/ (installer ~90MB)
# Run installer, default path: C:\msys64
# After install, UCRT64 terminal opens automatically
#
# IMPORTANT: MSYS2 is stored on E: drive to save C: space:
#   Actual location:  E:\SteamLibrary\IInstalled_Programs\MSYS2_Environment\msys64
#   Junction link:    C:\msys64  →  E:\...\MSYS2_Environment\msys64
# The junction ensures all paths referencing C:\msys64 still work.
```

### Step 2: Update MSYS2 (in UCRT64 terminal)
```bash
pacman -Syu
# If terminal closes, reopen "UCRT64" from Start Menu, then:
pacman -Su
```

### Step 3: Install Compiler & Build Tools
```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkg-config \
  mingw-w64-ucrt-x86_64-make \
  git
```

### Step 4: Install Game Dev Libraries
```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-glfw \
  mingw-w64-ucrt-x86_64-glm \
  mingw-w64-ucrt-x86_64-assimp \
  mingw-w64-ucrt-x86_64-bullet \
  mingw-w64-ucrt-x86_64-openal \
  mingw-w64-ucrt-x86_64-nlohmann-json
```

### Step 5: Generate GLAD (OpenGL 4.6 Core)
```bash
# Option A: Download from https://glad.dav1d.de/
#   - Language: C/C++
#   - Specification: OpenGL
#   - API: gl Version 4.6
#   - Profile: Core
#   - Check "Generate a loader"
#   - Click Generate, download zip
#   - Extract to src/vendor/glad/

# Option B: Install python-glad in MSYS2 and generate:
pacman -S mingw-w64-ucrt-x86_64-python-glad
python -m glad --generator=c --api=gl:4.6 --profile=core --out-path=src/vendor/glad
```

### Step 6: Download Single-Header Libraries
```bash
# stb_image
mkdir -p src/vendor/stb
curl -L -o src/vendor/stb/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
curl -L -o src/vendor/stb/stb_image_write.h https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h

# Dear ImGui
mkdir -p src/vendor/imgui
curl -L -o src/vendor/imgui/imgui.h https://raw.githubusercontent.com/ocornut/imgui/master/imgui.h
curl -L -o src/vendor/imgui/imgui.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui.cpp
curl -L -o src/vendor/imgui/imgui_impl_glfw.h https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_glfw.h
curl -L -o src/vendor/imgui/imgui_impl_glfw.cpp https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_glfw.cpp
curl -L -o src/vendor/imgui/imgui_impl_opengl3.h https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_opengl3.h
curl -L -o src/vendor/imgui/imgui_impl_opengl3.cpp https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_opengl3.cpp
curl -L -o src/vendor/imgui/imconfig.h https://raw.githubusercontent.com/ocornut/imgui/master/imconfig.h
curl -L -o src/vendor/imgui/imgui_internal.h https://raw.githubusercontent.com/ocornut/imgui/master/imgui_internal.h
curl -L -o src/vendor/imgui/imgui_draw.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_draw.cpp
curl -L -o src/vendor/imgui/imgui_tables.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_tables.cpp
curl -L -o src/vendor/imgui/imgui_widgets.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_widgets.cpp
```

### Step 7: Build Project
```bash
cd /e/SteamLibrary/Programming/blank
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Step 8: Run
```bash
./build/PhantomStrike.exe
```

### IMPORTANT: Add MSYS2 to PATH
To run the built .exe outside MSYS2 terminal, add to Windows PATH:
```
C:\msys64\ucrt64\bin
```
This ensures the .exe can find the MinGW runtime DLLs (libgcc, libstdc++, libwinpthread).

### IMPORTANT: MSYS2 Storage on E: Drive
MSYS2 is stored on E: drive (not C:) using a directory junction:
- **Actual files**: `E:\SteamLibrary\IInstalled_Programs\MSYS2_Environment\msys64`
- **Junction link**: `C:\msys64` → points to the actual location above
- All tools, bash, and build scripts reference `C:\msys64` which resolves through the junction
- **Do NOT delete** the junction at `C:\msys64` or MSYS2 will break
- To remove MSYS2: delete both the junction (`C:\msys64`) and the actual files (`E:\...\MSYS2_Environment`)

---

## 10. RISK MITIGATION

| Risk | Mitigation |
|------|------------|
| OpenGL driver issues | Test on multiple GPUs, fallback paths |
| Complex rendering bugs | Start simple, iterate, use RenderDoc |
| Asset pipeline complexity | Use Assimp for universal format support |
| Performance bottlenecks | Profile with NVIDIA Nsight, Tracy |
| Scope creep | Stick to phase plan, MVP first |
| Physics bugs | Use Bullet's debug draw, visualize colliders |
| MSYS2 library mismatch | Always use UCRT64 environment, never mix |

---

**This plan is a living document. Update as development progresses.**

*Next action: Start Phase 1 - Core Engine (window wrapper, input system, shader loading, FPS camera, game loop)*
