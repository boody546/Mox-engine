// ═══════════════════════════════════════════════════════════════════
//  Nova2D Engine — Implementation
// ═══════════════════════════════════════════════════════════════════

#include "core/Engine.h"
#include "input/InputManager.h"
#include "rendering/Renderer2D.h"
#include "rendering/Camera2D.h"
#include "scene/SceneTree.h"
#include "physics/PhysicsWorld.h"
#include "audio/AudioManager.h"
#include "assets/ResourceManager.h"

#include <SDL.h>

namespace Nova {

Engine* Engine::instance_ = nullptr;

Engine::Engine() {
    NOVA_ASSERT(!instance_, "Only one Engine instance allowed!");
    instance_ = this;
}

Engine::~Engine() {
    Shutdown();
    instance_ = nullptr;
}

bool Engine::Init(const EngineConfig& config) {
    config_ = config;
    
    NOVA_LOG("═══════════════════════════════════════════");
    NOVA_LOG("  Nova2D Engine v0.1.0");
    NOVA_LOG("  Initializing...");
    NOVA_LOG("═══════════════════════════════════════════");

    // ─── Initialize SDL2 ────────────────────────────────────
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        NOVA_ERROR("SDL2 init failed: ", SDL_GetError());
        return false;
    }
    NOVA_LOG("SDL2 initialized successfully");

    // ─── Create Window ──────────────────────────────────────
    Uint32 windowFlags = SDL_WINDOW_SHOWN;
    if (config_.resizable)  windowFlags |= SDL_WINDOW_RESIZABLE;
    if (config_.fullscreen) windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;

    window_ = SDL_CreateWindow(
        config_.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config_.windowWidth,
        config_.windowHeight,
        windowFlags
    );

    if (!window_) {
        NOVA_ERROR("Window creation failed: ", SDL_GetError());
        return false;
    }
    
    windowWidth_ = config_.windowWidth;
    windowHeight_ = config_.windowHeight;
    NOVA_LOG("Window created: ", windowWidth_, "x", windowHeight_);

    // ─── Create SDL Renderer ────────────────────────────────
    Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
    if (config_.vsync) rendererFlags |= SDL_RENDERER_PRESENTVSYNC;

    if (!config_.renderDriver.empty() && config_.renderDriver != "auto") {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, config_.renderDriver.c_str());
    }

    sdlRenderer_ = SDL_CreateRenderer(window_, -1, rendererFlags);
    if (!sdlRenderer_) {
        // Fallback to default renderer if specific driver fails
        SDL_ClearError();
        sdlRenderer_ = SDL_CreateRenderer(window_, -1, rendererFlags);
    }
    if (!sdlRenderer_) {
        NOVA_ERROR("Renderer creation failed: ", SDL_GetError());
        return false;
    }

    // Enable alpha blending by default
    SDL_SetRenderDrawBlendMode(sdlRenderer_, SDL_BLENDMODE_BLEND);
    
    // Log renderer info
    SDL_RendererInfo info;
    SDL_GetRendererInfo(sdlRenderer_, &info);
    NOVA_LOG("GPU Renderer Backend: ", info.name, " (Hardware Accelerated)");

    // ─── Initialize Subsystems ──────────────────────────────
    input_ = std::make_unique<InputManager>();
    NOVA_LOG("Input system initialized");

    resources_ = std::make_unique<ResourceManager>(sdlRenderer_);
    NOVA_LOG("Resource manager initialized");

    renderer_ = std::make_unique<Renderer2D>(sdlRenderer_);
    NOVA_LOG("Renderer2D initialized");

    camera_ = std::make_unique<Camera2D>(
        Vec2(windowWidth_ / 2.0f, windowHeight_ / 2.0f),
        Vec2((float)windowWidth_, (float)windowHeight_)
    );
    NOVA_LOG("Camera2D initialized");

    physics_ = std::make_unique<PhysicsWorld>();
    NOVA_LOG("Physics world initialized");

    audio_ = std::make_unique<AudioManager>();
    audio_->Init();
    NOVA_LOG("Audio system initialized");

    sceneTree_ = std::make_unique<SceneTree>(this);
    NOVA_LOG("Scene tree initialized");

    NOVA_LOG("═══════════════════════════════════════════");
    NOVA_LOG("  Engine ready! ✓");
    NOVA_LOG("═══════════════════════════════════════════");

    return true;
}

void Engine::Run() {
    running_ = true;

    // Call user init callback
    if (onInit_) onInit_();

    // Initialize scene tree
    sceneTree_->Init();

    // Fixed timestep variables
    const float fixedDt = 1.0f / config_.targetFPS;
    Uint64 previousTime = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float accumulator = 0.0f;

    // FPS tracking
    float fpsTimer = 0.0f;
    int   frameCount = 0;

    NOVA_LOG("Game loop started at ", config_.targetFPS, " target FPS");

    while (running_) {
        // ─── Calculate Delta Time ───────────────────────────
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float rawDt = static_cast<float>(currentTime - previousTime) / frequency;
        previousTime = currentTime;

        // Clamp delta time to avoid spiral of death
        if (rawDt > 0.25f) rawDt = 0.25f;
        
        deltaTime_ = rawDt;
        totalTime_ += rawDt;

        // ─── FPS Counter ────────────────────────────────────
        fpsTimer += rawDt;
        frameCount++;
        if (fpsTimer >= 1.0f) {
            fps_ = static_cast<float>(frameCount) / fpsTimer;
            frameCount = 0;
            fpsTimer = 0.0f;
        }

        // ─── Process Input ──────────────────────────────────
        ProcessEvents();
        if (!running_) break;

        // ─── Fixed Timestep Update ──────────────────────────
        accumulator += rawDt;
        while (accumulator >= fixedDt) {
            Update(fixedDt);
            accumulator -= fixedDt;
        }

        // ─── Render ─────────────────────────────────────────
        Render();
    }

    NOVA_LOG("Game loop ended");
}

void Engine::ProcessEvents() {
    input_->PreUpdate();
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                Quit();
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    windowWidth_ = event.window.data1;
                    windowHeight_ = event.window.data2;
                    camera_->SetViewportSize(Vec2((float)windowWidth_, (float)windowHeight_));
                    NOVA_TRACE("Window resized: ", windowWidth_, "x", windowHeight_);
                }
                break;

            default:
                break;
        }
        
        input_->ProcessEvent(event);
    }
}

void Engine::Update(float dt) {
    // Update camera
    camera_->Update(dt);

    // Update physics
    physics_->Step(dt);

    // Update scene tree
    sceneTree_->Update(dt);

    // User update callback
    if (onUpdate_) onUpdate_(dt);
}

void Engine::Render() {
    // Clear screen
    auto& c = config_.clearColor;
    SDL_SetRenderDrawColor(sdlRenderer_, c.R8(), c.G8(), c.B8(), c.A8());
    SDL_RenderClear(sdlRenderer_);

    // Apply camera transform
    renderer_->SetCamera(camera_.get());

    // Render scene tree
    sceneTree_->Render(renderer_.get());

    // User render callback
    if (onRender_) onRender_();

    // Present
    SDL_RenderPresent(sdlRenderer_);
}

void Engine::Quit() {
    running_ = false;
    NOVA_LOG("Quit requested");
}

void Engine::Shutdown() {
    NOVA_LOG("Shutting down engine...");

    // Destroy subsystems in reverse order
    sceneTree_.reset();
    audio_.reset();
    physics_.reset();
    camera_.reset();
    renderer_.reset();
    resources_.reset();
    input_.reset();

    if (sdlRenderer_) {
        SDL_DestroyRenderer(sdlRenderer_);
        sdlRenderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
    NOVA_LOG("Engine shut down successfully");
}

} // namespace Nova
