// ═══════════════════════════════════════════════════════════════════
//  Mox Engine — Entry Point (v4.0)
//  Parses and renders scenes/main.json from active project.
//  Renders empty dark canvas (#121218) if no scene/project is passed.
// ═══════════════════════════════════════════════════════════════════

#include "core/Engine.h"
#include "core/Logger.h"
#include "input/InputManager.h"
#include "rendering/Renderer2D.h"
#include "rendering/Camera2D.h"
#include "scene/Node.h"
#include "scene/Node2D.h"
#include "scene/SceneTree.h"
#include "scene/SceneLoader.h"
#include "physics/PhysicsWorld.h"

#include <string>
#include <cstring>
#include <algorithm>

using namespace Nova;

// ═══════════════════════════════════════════════════════════════════
//  CLI Argument Parser
// ═══════════════════════════════════════════════════════════════════
struct CliArgs {
    std::string projectPath;   // --project "..."
    std::string scenePath;     // --scene   "..."  (relative to project)
    bool hasProject = false;
};

static CliArgs ParseArgs(int argc, char* argv[]) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--project") == 0 && i + 1 < argc) {
            args.projectPath = argv[++i];
            args.hasProject  = true;
        } else if (std::strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            args.scenePath = argv[++i];
        } else if (i == 1 && argv[i][0] != '-') {
            // Positional first argument treated as --project
            args.projectPath = argv[i];
            args.hasProject  = true;
        }
    }
    if (args.scenePath.empty()) {
        args.scenePath = "scenes/main.json";
    }
    return args;
}

// ═══════════════════════════════════════════════════════════════════
//  Empty Scene Handler
// ═══════════════════════════════════════════════════════════════════
static void SetupEmptySceneLoop(Engine& engine, const std::string& statusMsg) {
    engine.SetOnUpdate([&](float dt) {
        (void)dt;
        if (engine.GetInput()->IsKeyJustPressed(SDL_SCANCODE_ESCAPE))
            engine.Quit();
    });

    engine.SetOnRender([&, statusMsg]() {
        auto* r = engine.GetRenderer();
        // Draw subtle background grid for empty canvas
        Color grid = Color::FromRGBA8(30, 30, 40, 40);
        for (int x = -2000; x <= 4000; x += 64)
            r->DrawLine(Vec2((float)x, -2000), Vec2((float)x, 4000), grid);
        for (int y = -2000; y <= 4000; y += 64)
            r->DrawLine(Vec2(-2000, (float)y), Vec2(4000, (float)y), grid);

        std::string title = "Mox Engine — " + statusMsg + " | ESC: Quit";
        SDL_SetWindowTitle(engine.GetWindow(), title.c_str());
    });
}

// ═══════════════════════════════════════════════════════════════════
//  Project Scene Loop Callbacks
// ═══════════════════════════════════════════════════════════════════
static void SetupProjectLoop(Engine& engine,
                              const std::string& projectPath,
                              const std::string& scenePath) {
    engine.SetOnUpdate([&](float dt) {
        (void)dt;
        // Zoom with scroll wheel
        float scroll = engine.GetInput()->GetMouseScrollDelta();
        if (scroll != 0) engine.GetCamera()->ZoomBy(scroll * 0.1f);
        // ESC -> quit
        if (engine.GetInput()->IsKeyJustPressed(SDL_SCANCODE_ESCAPE))
            engine.Quit();
    });

    engine.SetOnRender([&]() {
        auto* r = engine.GetRenderer();
        // Draw world-space grid
        Color grid = Color::FromRGBA8(255, 255, 255, 8);
        for (int x = -2000; x <= 4000; x += 64)
            r->DrawLine(Vec2((float)x, -2000), Vec2((float)x, 4000), grid);
        for (int y = -2000; y <= 4000; y += 64)
            r->DrawLine(Vec2(-2000, (float)y), Vec2(4000, (float)y), grid);

        std::string title = "Mox Engine | " + projectPath +
            " | FPS: " + std::to_string((int)engine.GetFPS()) +
            " | Scroll: Zoom | ESC: Quit";
        SDL_SetWindowTitle(engine.GetWindow(), title.c_str());
    });
}

// ═══════════════════════════════════════════════════════════════════
//  main()
// ═══════════════════════════════════════════════════════════════════
int main(int argc, char* argv[]) {
    CliArgs cli = ParseArgs(argc, argv);

    Engine engine;
    EngineConfig config;

    config.title       = cli.hasProject ? ("Mox Engine — " + cli.projectPath) : "Mox Engine — Empty Scene";
    config.windowWidth = 1280;
    config.windowHeight= 720;
    config.clearColor  = Color::FromRGBA8(18, 18, 24); // #121218 dark canvas
    config.vsync       = true;
    config.renderDriver= "direct3d11";

    if (!engine.Init(config)) {
        NOVA_ERROR("Engine initialization failed!");
        return -1;
    }

    NOVA_LOG("═══════════════════════════════════════════════════");
    if (cli.hasProject) {
        NOVA_LOG("  Mox Engine — Project Mode");
        NOVA_LOG("  Project : ", cli.projectPath);
        NOVA_LOG("  Scene   : ", cli.scenePath);
    } else {
        NOVA_LOG("  Mox Engine — Empty Scene (no --project/--scene supplied)");
    }
    NOVA_LOG("═══════════════════════════════════════════════════");

    if (cli.hasProject) {
        bool ok = SceneLoader::LoadFromFile(cli.projectPath, cli.scenePath, &engine);
        if (!ok) {
            NOVA_ERROR("SceneLoader failed to load: ", cli.projectPath, "/", cli.scenePath);
            SetupEmptySceneLoop(engine, "Empty Scene - No nodes present");
        } else {
            SetupProjectLoop(engine, cli.projectPath, cli.scenePath);
        }
    } else {
        SetupEmptySceneLoop(engine, "Empty Scene - No nodes present");
    }

    engine.Run();
    return 0;
}
