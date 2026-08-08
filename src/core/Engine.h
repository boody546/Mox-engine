#pragma once
// ═══════════════════════════════════════════════════════════════════
//  Nova2D Engine — Main engine class owning the game loop
// ═══════════════════════════════════════════════════════════════════

#include "core/Math.h"
#include "core/Logger.h"

#include <SDL.h>
#include <string>
#include <memory>
#include <functional>

namespace Nova {

// Forward declarations
class InputManager;
class Renderer2D;
class Camera2D;
class SceneTree;
class PhysicsWorld;
class AudioManager;
class ResourceManager;

struct EngineConfig {
    std::string title = "Nova2D Game";
    int windowWidth = 1280;
    int windowHeight = 720;
    bool fullscreen = false;
    bool vsync = true;
    bool resizable = true;
    int targetFPS = 60;
    std::string renderDriver = "direct3d11"; // Options: "direct3d11" (DirectX 11), "opengl" (OpenGL), "direct3d", "auto"
    Color clearColor = Color::FromRGBA8(20, 20, 30);
};

class Engine {
public:
    Engine();
    ~Engine();

    // No copy/move
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // ─── Lifecycle ──────────────────────────────────────────
    bool Init(const EngineConfig& config = {});
    void Run();
    void Shutdown();
    void Quit();

    // ─── Subsystem Access ───────────────────────────────────
    InputManager*    GetInput()     const { return input_.get(); }
    Renderer2D*      GetRenderer()  const { return renderer_.get(); }
    Camera2D*        GetCamera()    const { return camera_.get(); }
    SceneTree*       GetSceneTree() const { return sceneTree_.get(); }
    PhysicsWorld*    GetPhysics()   const { return physics_.get(); }
    AudioManager*    GetAudio()     const { return audio_.get(); }
    ResourceManager* GetResources() const { return resources_.get(); }

    // ─── Callbacks ──────────────────────────────────────────
    using UpdateCallback = std::function<void(float dt)>;
    using RenderCallback = std::function<void()>;
    using InitCallback   = std::function<void()>;

    void SetOnInit(InitCallback cb)     { onInit_ = std::move(cb); }
    void SetOnUpdate(UpdateCallback cb) { onUpdate_ = std::move(cb); }
    void SetOnRender(RenderCallback cb) { onRender_ = std::move(cb); }

    // ─── Info ───────────────────────────────────────────────
    float     GetDeltaTime()   const { return deltaTime_; }
    float     GetFPS()         const { return fps_; }
    float     GetTime()        const { return totalTime_; }
    int       GetWindowWidth() const { return windowWidth_; }
    int       GetWindowHeight() const { return windowHeight_; }
    bool      IsRunning()      const { return running_; }
    SDL_Window*   GetWindow()  const { return window_; }
    SDL_Renderer* GetSDLRenderer() const { return sdlRenderer_; }

    // ─── Singleton Access ───────────────────────────────────
    static Engine* Instance() { return instance_; }

private:
    void ProcessEvents();
    void Update(float dt);
    void Render();

    static Engine* instance_;

    SDL_Window*   window_ = nullptr;
    SDL_Renderer* sdlRenderer_ = nullptr;

    // Subsystems
    std::unique_ptr<InputManager>    input_;
    std::unique_ptr<Renderer2D>      renderer_;
    std::unique_ptr<Camera2D>        camera_;
    std::unique_ptr<SceneTree>       sceneTree_;
    std::unique_ptr<PhysicsWorld>    physics_;
    std::unique_ptr<AudioManager>    audio_;
    std::unique_ptr<ResourceManager> resources_;

    // Callbacks
    InitCallback   onInit_;
    UpdateCallback onUpdate_;
    RenderCallback onRender_;

    // State
    bool  running_ = false;
    float deltaTime_ = 0.0f;
    float totalTime_ = 0.0f;
    float fps_ = 0.0f;
    int   windowWidth_ = 0;
    int   windowHeight_ = 0;
    EngineConfig config_;
};

} // namespace Nova
