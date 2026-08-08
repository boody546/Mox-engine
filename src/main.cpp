// ═══════════════════════════════════════════════════════════════════
//  Nova2D Engine — Entry Point
//  Demonstrates engine capabilities with a visual demo
// ═══════════════════════════════════════════════════════════════════

#include "core/Engine.h"
#include "core/Logger.h"
#include "input/InputManager.h"
#include "rendering/Renderer2D.h"
#include "rendering/Camera2D.h"
#include "rendering/ParticleSystem.h"
#include "rendering/Texture.h"
#include "assets/ResourceManager.h"
#include "scene/Node.h"
#include "scene/SceneTree.h"
#include "scene/ScriptNode.h"
#include "physics/PhysicsWorld.h"
#include "scripting/Lexer.h"
#include "scripting/Parser.h"
#include "scripting/Interpreter.h"

using namespace Nova;

// ─── Demo: Custom Node that renders a colored rectangle ─────────
class ColorRect : public Node {
public:
    Vec2 size{50, 50};
    Color color = Color::CornflowerBlue();
    Vec2 velocity;

    ColorRect(const std::string& name, Vec2 sz, Color col)
        : Node(name), size(sz), color(col) {}

    void _Draw(Renderer2D* renderer) override {
        Vec2 pos = GetGlobalPosition();
        renderer->DrawRect(Rect2(pos - size * 0.5f, size), color, true);
    }

    std::string GetType() const override { return "ColorRect"; }
};

// ─── Demo: Scripted Box controlled by NovaScript ────────────────
class ScriptedBox : public ScriptNode {
public:
    Vec2 size{50, 50};
    Color color = Color::FromRGBA8(255, 220, 50);

    ScriptedBox(const std::string& name) : ScriptNode(name) {}

    void _Draw(Renderer2D* renderer) override {
        Vec2 pos = GetGlobalPosition();
        float rot = GetRotation();
        renderer->DrawRect(Rect2(pos - size * 0.5f, size), color, true);
        
        // Draw rotation pointer line
        float rad = rot * 3.14159265f / 180.0f;
        Vec2 rotDir(std::cos(rad), std::sin(rad));
        renderer->DrawLine(pos, pos + rotDir * 35.0f, Color::White());
    }

    std::string GetType() const override { return "ScriptedBox"; }
};

// ─── Demo: Player node controlled by input ──────────────────────
class PlayerNode : public Node {
public:
    float speed = 300.0f;
    Vec2 size{40, 40};
    Color color = Color::FromRGBA8(100, 220, 255);

    PlayerNode() : Node("Player") {}

    void _Update(float dt) override {
        auto* input = Engine::Instance()->GetInput();
        Vec2 dir;
        if (input->IsKeyDown(SDL_SCANCODE_W) || input->IsKeyDown(SDL_SCANCODE_UP))    dir.y -= 1;
        if (input->IsKeyDown(SDL_SCANCODE_S) || input->IsKeyDown(SDL_SCANCODE_DOWN))  dir.y += 1;
        if (input->IsKeyDown(SDL_SCANCODE_A) || input->IsKeyDown(SDL_SCANCODE_LEFT))  dir.x -= 1;
        if (input->IsKeyDown(SDL_SCANCODE_D) || input->IsKeyDown(SDL_SCANCODE_RIGHT)) dir.x += 1;

        if (!dir.IsZero()) {
            dir.Normalize();
            transform_.position += dir * speed * dt;
        }
    }

    void _Draw(Renderer2D* renderer) override {
        Vec2 pos = GetGlobalPosition();
        // Draw glow effect
        for (int i = 3; i >= 0; i--) {
            float expand = i * 4.0f;
            Color glow = color.WithAlpha(0.15f - i * 0.03f);
            renderer->DrawRect(Rect2(pos - (size + Vec2(expand)) * 0.5f, size + Vec2(expand)), glow);
        }
        // Draw player
        renderer->DrawRect(Rect2(pos - size * 0.5f, size), color);
        // Draw inner highlight
        Vec2 innerSize = size * 0.6f;
        Color highlight = Color::FromRGBA8(180, 240, 255, 100);
        renderer->DrawRect(Rect2(pos - innerSize * 0.5f, innerSize), highlight);
    }
};

// ─── Main ───────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    Engine engine;

    EngineConfig config;
    config.title = "Mox Engine — Demo (DirectX 11 / OpenGL)";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.clearColor = Color::FromRGBA8(15, 15, 25);
    config.vsync = true;
    config.renderDriver = "direct3d11"; // DirectX 11 hardware acceleration

    if (!engine.Init(config)) {
        NOVA_ERROR("Engine initialization failed!");
        return -1;
    }

    // ─── Create Demo Scene ──────────────────────────────────
    auto* player = new PlayerNode();
    player->SetPosition(Vec2(640, 360));

    // Add some colorful boxes
    auto* box1 = new ColorRect("RedBox", Vec2(60, 60), Color::FromRGBA8(255, 80, 80));
    box1->SetPosition(Vec2(300, 300));
    box1->velocity = Vec2(80, 60);

    auto* box2 = new ColorRect("GreenBox", Vec2(45, 45), Color::FromRGBA8(80, 255, 120));
    box2->SetPosition(Vec2(800, 200));
    box2->velocity = Vec2(-70, 90);

    auto* box3 = new ColorRect("PurpleBox", Vec2(55, 55), Color::FromRGBA8(180, 80, 255));
    box3->SetPosition(Vec2(500, 500));
    box3->velocity = Vec2(100, -50);

    // Add ScriptedBox controlled by scripts/player.nova
    auto* scriptBox = new ScriptedBox("ScriptedGoldBox");
    scriptBox->SetPosition(Vec2(400, 250));
    scriptBox->LoadScript("scripts/player.nova");

    engine.GetSceneTree()->AddToRoot(player);
    engine.GetSceneTree()->AddToRoot(box1);
    engine.GetSceneTree()->AddToRoot(box2);
    engine.GetSceneTree()->AddToRoot(box3);
    engine.GetSceneTree()->AddToRoot(scriptBox);

    // Camera follows the player
    engine.GetCamera()->SetFollowTarget(&player->GetTransform().position, 5.0f);

    // ─── Create Particle System ─────────────────────────────
    ParticleConfig particleConfig;
    particleConfig.emitRate = 60.0f;
    particleConfig.lifetime = 1.5f;
    particleConfig.startSize = 6.0f;
    particleConfig.endSize = 0.0f;
    particleConfig.startColor = Color::FromRGBA8(255, 200, 50);
    particleConfig.endColor = Color::FromRGBA8(255, 50, 50, 0);
    particleConfig.emitSpeed = 80.0f;
    particleConfig.emitAngle = 360.0f;
    particleConfig.gravity = Vec2(0, -50);
    ParticleSystem particles(particleConfig);
    particles.Start();

    // ─── Test NovaScript ────────────────────────────────────
    NOVA_LOG("═══════════════════════════════════════════");
    NOVA_LOG("  Testing NovaScript...");
    NOVA_LOG("═══════════════════════════════════════════");

    std::string testScript = R"(
# NovaScript Demo
var name = "Nova2D"
var version = 0.1
print("Welcome to " + name + " v" + str(version) + "!")

func fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

var result = fibonacci(10)
print("Fibonacci(10) = " + str(result))

var items = [1, 2, 3, 4, 5]
var total = 0
for item in items:
    total += item
print("Sum of " + str(items) + " = " + str(total))

func greet(who):
    return "Hello, " + who + "!"

print(greet("Game Developer"))
print("Engine ready! Let's make games!")
)";

    Lexer lexer(testScript, "demo.nova");
    auto tokens = lexer.Tokenize();
    if (lexer.HasErrors()) {
        for (auto& err : lexer.GetErrors()) NOVA_ERROR("Lexer: ", err);
    }

    Parser parser(tokens, "demo.nova");
    auto script = parser.Parse();
    if (parser.HasErrors()) {
        for (auto& err : parser.GetErrors()) NOVA_ERROR("Parser: ", err);
    }

    Interpreter interpreter(&engine);
    interpreter.Execute(script.get());
    if (interpreter.HasErrors()) {
        for (auto& err : interpreter.GetErrors()) NOVA_ERROR("Runtime: ", err);
    }

    NOVA_LOG("═══════════════════════════════════════════");
    NOVA_LOG("  NovaScript test complete!");
    NOVA_LOG("═══════════════════════════════════════════");

    // ─── Game Loop Callbacks ────────────────────────────────
    float totalTime = 0;

    engine.SetOnUpdate([&](float dt) {
        totalTime += dt;

        // Bounce boxes around the screen
        auto bounceBox = [&](ColorRect* box) {
            box->SetPosition(box->GetPosition() + box->velocity * dt);
            Vec2 pos = box->GetPosition();
            if (pos.x < 50 || pos.x > 1230) { box->velocity.x *= -1; }
            if (pos.y < 50 || pos.y > 670)  { box->velocity.y *= -1; }
        };
        bounceBox(box1);
        bounceBox(box2);
        bounceBox(box3);

        // Particles follow the player
        particles.SetPosition(player->GetPosition());
        particles.Update(dt);

        // Screen shake on space
        if (engine.GetInput()->IsKeyJustPressed(SDL_SCANCODE_SPACE)) {
            engine.GetCamera()->Shake(8.0f, 0.3f);
        }

        // Zoom with mouse wheel
        float scroll = engine.GetInput()->GetMouseScrollDelta();
        if (scroll != 0) {
            engine.GetCamera()->ZoomBy(scroll * 0.1f);
        }
    });

    engine.SetOnRender([&]() {
        auto* renderer = engine.GetRenderer();

        // Render particles
        particles.Render(renderer);

        // Draw grid pattern in background
        Color gridColor = Color::FromRGBA8(255, 255, 255, 15);
        for (int x = -1000; x <= 2000; x += 100) {
            renderer->DrawLine(Vec2((float)x, -1000), Vec2((float)x, 2000), gridColor);
        }
        for (int y = -1000; y <= 2000; y += 100) {
            renderer->DrawLine(Vec2(-1000, (float)y), Vec2(2000, (float)y), gridColor);
        }

        // HUD (drawn in screen space — bypass camera)
        // We draw it using SDL directly for simplicity
        auto* sdlRenderer = engine.GetSDLRenderer();
        SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 180);

        // FPS display position
        char fpsText[64];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.0f", engine.GetFPS());
        // Note: proper text rendering requires TextRenderer (future feature)
        // For now, FPS is displayed in the window title
        std::string title = "Nova2D Engine | FPS: " + std::to_string((int)engine.GetFPS()) +
                           " | WASD/Arrows: Move | Scroll: Zoom | Space: Shake";
        SDL_SetWindowTitle(engine.GetWindow(), title.c_str());
    });

    // ─── Run! ───────────────────────────────────────────────
    engine.Run();

    return 0;
}
