// ═══════════════════════════════════════════════════════════════════
//  Mox Engine — SceneLoader.cpp
//  Parses scenes/main.json produced by HUB/editor.py and populates
//  the running SceneTree with live Node objects.
//
//  JSON format (editor v4.0):
//  {
//    "version": "4.0",
//    "scene": [
//      { "id":"node_root", "name":"MainScene", "type":"Node2D",
//        "parent": null,
//        "tag":"Untagged", "layer":"Default",
//        "props": { "position_x":0, "position_y":0, "rotation":0,
//                   "scale_x":1, "scale_y":1, "visible":true,
//                   "z_index":0, … } },
//      { "id":"node_1", "name":"Player", "type":"CharacterBody2D",
//        "parent":"node_root", "script":"player_script.nova", … }
//    ]
//  }
// ═══════════════════════════════════════════════════════════════════

#include "scene/SceneLoader.h"
#include "scene/Node.h"
#include "scene/Node2D.h"
#include "scene/SceneTree.h"
#include "scene/ScriptNode.h"
#include "core/Engine.h"
#include "core/Logger.h"
#include "rendering/Renderer2D.h"
#include "rendering/Camera2D.h"
#include "rendering/ParticleSystem.h"
#include "assets/ResourceManager.h"
#include "physics/PhysicsWorld.h"

#define JSON_NOEXCEPTION
#include <json.hpp>          // nlohmann/json — already in third_party/
using json = nlohmann::json;

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace Nova {

// ─── Utility: parse "#rrggbb" / "#rrggbbaa" ───────────────────────
Color SceneLoader::_hexToColor(const std::string& hex) {
    if (hex.empty() || hex[0] != '#') return Color::White();
    std::string h = hex.substr(1);
    // Pad to 8 chars if needed
    if (h.size() == 3) {
        h = {h[0],h[0],h[1],h[1],h[2],h[2],'f','f'};
    } else if (h.size() == 6) {
        h += "ff";
    }
    if (h.size() < 8) return Color::White();
    try {
        uint8_t r = (uint8_t)std::stoul(h.substr(0,2), nullptr, 16);
        uint8_t g = (uint8_t)std::stoul(h.substr(2,2), nullptr, 16);
        uint8_t b = (uint8_t)std::stoul(h.substr(4,2), nullptr, 16);
        uint8_t a = (uint8_t)std::stoul(h.substr(6,2), nullptr, 16);
        return Color::FromRGBA8(r, g, b, a);
    } catch (...) {
        return Color::White();
    }
}

// ─── Helper: safely read a value from json props ──────────────────
template<typename T>
static T jget(const json& j, const std::string& key, T def) {
    if (j.contains(key) && !j[key].is_null()) {
        try { return j[key].get<T>(); }
        catch (...) {}
    }
    return def;
}

// ─── Apply generic Node properties from props object ─────────────
void SceneLoader::_applyCommon(Node* node, const void* jPropsPtr) {
    if (!node || !jPropsPtr) return;
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    float px = jget<float>(p, "position_x", 0.0f);
    float py = jget<float>(p, "position_y", 0.0f);
    node->SetPosition(Vec2(px, py));

    float rot = jget<float>(p, "rotation", 0.0f);
    node->SetRotation(rot);

    float sx = jget<float>(p, "scale_x", 1.0f);
    float sy = jget<float>(p, "scale_y", 1.0f);
    node->SetScale(Vec2(sx, sy));

    bool vis = jget<bool>(p, "visible", true);
    node->SetVisible(vis);

    int z = jget<int>(p, "z_index", 0);
    node->SetZIndex(z);
}

// ─── Per-type factories ───────────────────────────────────────────
Node* SceneLoader::_createNode(const std::string& type, const std::string& name) {
    if (type == "Node2D")            return new Node2D(name);
    if (type == "Sprite2D")          return new Sprite2D(name);
    if (type == "Light2D")           return new Light2D(name);
    if (type == "Camera2D")          return new Camera2DNode(name);
    if (type == "CharacterBody2D")   return new CharacterBody2DNode(name);
    if (type == "RigidBody2D")       return new RigidBody2DNode(name);
    if (type == "StaticBody2D")      return new StaticBody2DNode(name);
    if (type == "Area2D")            return new StaticBody2DNode(name);   // lightweight sensor
    if (type == "TileMap")           return new TileMapNode(name);
    if (type == "CPUParticles2D")    return new CPUParticles2DNode(name);
    if (type == "AudioStreamPlayer") return new AudioNode(name);
    if (type == "AudioStreamPlayer2D") return new AudioNode(name);
    if (type == "CollisionShape2D")  return new Node2D(name);             // visual only
    if (type == "CollisionPolygon2D")return new Node2D(name);
    if (type == "AnimatedSprite2D")  return new Sprite2D(name);           // treated as sprite
    if (type == "CanvasModulate")    return new Node2D(name);
    if (type == "Label")             return new Node2D(name);
    if (type == "Button")            return new Node2D(name);
    // Fallback: generic Node2D
    NOVA_LOG("SceneLoader: unknown type '", type, "' — creating Node2D");
    return new Node2D(name);
}

// ─── Sprite2D ─────────────────────────────────────────────────────
void SceneLoader::_loadSprite2D(Node* node, const void* jPropsPtr,
                                 const std::string& projectPath, Engine* engine) {
    auto* sprite = static_cast<Sprite2D*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    std::string texRelPath = jget<std::string>(p, "texture", "");
    if (!texRelPath.empty() && engine && engine->GetResources()) {
        // Try <project>/assets/<filename>
        namespace fs = std::filesystem;
        std::string absPath = (fs::path(projectPath) / "assets" / fs::path(texRelPath).filename()).string();
        if (!fs::exists(absPath)) {
            // Try the raw value as absolute
            absPath = texRelPath;
        }
        if (fs::exists(absPath)) {
            sprite->SetTexturePath(absPath);
            // Preload into ResourceManager cache
            engine->GetResources()->LoadTexture(absPath);
        } else {
            NOVA_LOG("SceneLoader: texture not found: ", absPath);
        }
    }

    std::string modStr = jget<std::string>(p, "modulate", "#ffffff");
    sprite->SetModulate(SceneLoader::_hexToColor(modStr));

    bool fh = jget<bool>(p, "flip_h", false);
    bool fv = jget<bool>(p, "flip_v", false);
    sprite->SetFlipH(fh);
    sprite->SetFlipV(fv);

    float w = jget<float>(p, "width",  64.0f);
    float h = jget<float>(p, "height", 64.0f);
    sprite->SetSize(Vec2(w, h));
}

// ─── Light2D ──────────────────────────────────────────────────────
void SceneLoader::_loadLight2D(Node* node, const void* jPropsPtr) {
    auto* light = static_cast<Light2D*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    std::string colStr = jget<std::string>(p, "color", "#ffffaa");
    light->SetLightColor(SceneLoader::_hexToColor(colStr));

    float radius = jget<float>(p, "radius", 150.0f);
    light->SetRadius(radius);

    float energy = jget<float>(p, "energy", 1.0f);
    light->SetEnergy(energy);
}

// ─── Camera2D ─────────────────────────────────────────────────────
void SceneLoader::_loadCamera2D(Node* node, const void* jPropsPtr, Engine* engine) {
    auto* cam = static_cast<Camera2DNode*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    float zoom = jget<float>(p, "zoom", 1.0f);
    cam->SetZoom(zoom);

    // Register as active camera
    if (engine && engine->GetCamera()) {
        Vec2 pos = node->GetPosition();
        engine->GetCamera()->SetPosition(pos);
        engine->GetCamera()->SetZoom(zoom);
    }
}

// ─── RigidBody2D ──────────────────────────────────────────────────
void SceneLoader::_loadRigidBody2D(Node* node, const void* jPropsPtr, Engine* engine) {
    auto* body = static_cast<RigidBody2DNode*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    if (!engine || !engine->GetPhysics()) return;
    auto* physics = engine->GetPhysics();

    std::string shape = jget<std::string>(p, "shape", "Box");
    float sx = jget<float>(p, "size_x", 40.0f);
    float sy = jget<float>(p, "size_y", 40.0f);
    float radius = jget<float>(p, "radius", 20.0f);
    float mass   = jget<float>(p, "mass",   1.0f);
    float gscale = jget<float>(p, "gravity_scale", 1.0f);
    float friction = jget<float>(p, "friction", 0.5f);
    float bounce   = jget<float>(p, "restitution", 0.2f);

    RigidBody2D* rb = physics->CreateBody();
    rb->position = node->GetPosition();
    rb->type     = BodyType::Dynamic;
    rb->SetMass(mass);
    rb->gravityScale = gscale;
    rb->friction     = friction;
    rb->bounce       = bounce;
    rb->userData     = node;

    Collider* col = nullptr;
    if (shape == "Circle") {
        auto* cc = new CircleCollider(radius);
        col = cc;
    } else {
        auto* ac = new AABBCollider(Vec2(sx, sy));
        col = ac;
    }
    rb->collider = col;

    body->SetPhysicsBody(rb, physics);
}

// ─── CharacterBody2D ──────────────────────────────────────────────
void SceneLoader::_loadCharacterBody2D(Node* node, const void* jPropsPtr, Engine* engine) {
    auto* charBody = static_cast<CharacterBody2DNode*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    charBody->speed        = jget<float>(p, "speed",        200.0f);
    charBody->jumpImpulse  = jget<float>(p, "jump_impulse", -400.0f);
    charBody->gravity      = jget<float>(p, "gravity",      980.0f);

    // Create physics body (kinematic)
    if (!engine || !engine->GetPhysics()) return;
    auto* physics = engine->GetPhysics();

    float sx = jget<float>(p, "size_x", 40.0f);
    float sy = jget<float>(p, "size_y", 60.0f);
    std::string shape = jget<std::string>(p, "shape", "Box");

    RigidBody2D* rb = physics->CreateBody();
    rb->position     = node->GetPosition();
    rb->type         = BodyType::Kinematic;
    rb->SetMass(1.0f);
    rb->gravityScale = 0.0f;   // CharacterBody2D manages gravity itself
    rb->userData     = node;

    Collider* col = nullptr;
    if (shape == "Circle") {
        col = new CircleCollider(jget<float>(p, "radius", 20.0f));
    } else {
        col = new AABBCollider(Vec2(sx, sy));
    }
    rb->collider = col;
    charBody->SetPhysicsBody(rb, physics);
}

// ─── StaticBody2D ─────────────────────────────────────────────────
void SceneLoader::_loadStaticBody2D(Node* node, const void* jPropsPtr, Engine* engine) {
    auto* sBody = static_cast<StaticBody2DNode*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    if (!engine || !engine->GetPhysics()) return;
    auto* physics = engine->GetPhysics();

    float sx = jget<float>(p, "size_x", 100.0f);
    float sy = jget<float>(p, "size_y",  20.0f);
    std::string shape = jget<std::string>(p, "shape", "Box");
    float radius = jget<float>(p, "radius", 20.0f);
    float friction   = jget<float>(p, "friction",    0.8f);
    float bounce     = jget<float>(p, "restitution", 0.0f);

    RigidBody2D* rb = physics->CreateBody();
    rb->position     = node->GetPosition();
    rb->type         = BodyType::Static;
    rb->SetMass(0.0f);   // infinite mass
    rb->gravityScale = 0.0f;
    rb->friction     = friction;
    rb->bounce       = bounce;
    rb->userData     = node;

    Collider* col = (shape == "Circle")
        ? (Collider*)new CircleCollider(radius)
        : (Collider*)new AABBCollider(Vec2(sx, sy));
    rb->collider = col;

    sBody->SetPhysicsBody(rb, physics);
}

// ─── TileMap ──────────────────────────────────────────────────────
void SceneLoader::_loadTileMap(Node* node, const void* jPropsPtr) {
    auto* tileMap = static_cast<TileMapNode*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    float cellSize = jget<float>(p, "cell_size", 32.0f);
    tileMap->SetCellSize(cellSize);

    // "tiles" is a dict: "col,row" → tile_id string
    if (p.contains("tiles") && p["tiles"].is_object()) {
        for (auto& [key, val] : p["tiles"].items()) {
            // parse "col,row"
            auto comma = key.find(',');
            if (comma == std::string::npos) continue;
            try {
                int col = std::stoi(key.substr(0, comma));
                int row = std::stoi(key.substr(comma + 1));
                std::string tid = val.is_string() ? val.get<std::string>() : "";
                tileMap->SetTile(col, row, tid);
            } catch (...) {}
        }
    }
}

// ─── CPUParticles2D ───────────────────────────────────────────────
void SceneLoader::_loadParticles(Node* node, const void* jPropsPtr) {
    auto* pNode = static_cast<CPUParticles2DNode*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    ParticleConfig cfg;
    cfg.emitPosition   = node->GetPosition();
    cfg.emitRate       = jget<float>(p, "amount",   30.0f);
    cfg.lifetime       = jget<float>(p, "lifetime",  1.5f);
    cfg.emitSpeed      = jget<float>(p, "speed",   100.0f);
    cfg.startColor     = SceneLoader::_hexToColor(jget<std::string>(p, "color_start", "#ffaa00"));
    cfg.endColor       = SceneLoader::_hexToColor(jget<std::string>(p, "color_end",   "#ff0000"));
    cfg.emitAngle      = 360.0f;
    cfg.gravity        = Vec2(0, 50.0f);
    cfg.startSize      = 6.0f;
    cfg.endSize        = 0.0f;
    cfg.maxParticles   = 500;

    pNode->SetParticleConfig(cfg);
    pNode->StartEmitting();
}

// ─── Audio ────────────────────────────────────────────────────────
void SceneLoader::_loadAudio(Node* node, const void* jPropsPtr,
                              const std::string& projectPath) {
    auto* aNode = static_cast<AudioNode*>(node);
    const json& p = *reinterpret_cast<const json*>(jPropsPtr);

    std::string stream = jget<std::string>(p, "stream", "");
    if (!stream.empty()) {
        namespace fs = std::filesystem;
        std::string abs = (fs::path(projectPath) / "assets" / fs::path(stream).filename()).string();
        aNode->SetStreamPath(abs);
        NOVA_LOG("AudioNode '", node->GetName(), "' stream: ", abs);
    }
    float vol = jget<float>(p, "volume_db", 0.0f);
    aNode->SetVolumeDb(vol);
}

// ═══════════════════════════════════════════════════════════════════
//  MAIN ENTRY POINT
// ═══════════════════════════════════════════════════════════════════
bool SceneLoader::LoadFromFile(const std::string& projectPath,
                                const std::string& relScenePath,
                                Engine* engine) {
    namespace fs = std::filesystem;

    // ── Build absolute scene path ───────────────────────────────────
    fs::path scenePath = fs::path(projectPath) / relScenePath;
    NOVA_LOG("SceneLoader: loading ", scenePath.string());

    // ── Read JSON ───────────────────────────────────────────────────
    std::ifstream f(scenePath.string());
    if (!f.is_open()) {
        NOVA_ERROR("SceneLoader: cannot open scene file: ", scenePath.string());
        return false;
    }

    json root;
    try {
        f >> root;
    } catch (const std::exception& e) {
        NOVA_ERROR("SceneLoader: JSON parse error: ", e.what());
        return false;
    }
    f.close();

    if (!root.contains("scene") || !root["scene"].is_array()) {
        NOVA_ERROR("SceneLoader: 'scene' array missing in JSON.");
        return false;
    }

    const auto& sceneArr = root["scene"];
    NOVA_LOG("SceneLoader: ", sceneArr.size(), " nodes found.");

    // ── Pass 1: Create all nodes ────────────────────────────────────
    // Map from JSON id string → live Node*
    std::unordered_map<std::string, Node*> nodeMap;

    for (const auto& jNode : sceneArr) {
        std::string id   = jget<std::string>(jNode, "id",   "");
        std::string name = jget<std::string>(jNode, "name", "Node");
        std::string type = jget<std::string>(jNode, "type", "Node2D");

        Node* node = _createNode(type, name);
        if (!node) continue;

        // Apply common transform/visibility props
        if (jNode.contains("props") && jNode["props"].is_object()) {
            const json& props = jNode["props"];
            _applyCommon(node, &props);

            // Per-type specialisation
            if (type == "Sprite2D" || type == "AnimatedSprite2D") {
                _loadSprite2D(node, &props, projectPath, engine);
            } else if (type == "Light2D") {
                _loadLight2D(node, &props);
            } else if (type == "Camera2D") {
                _loadCamera2D(node, &props, engine);
            } else if (type == "RigidBody2D") {
                _loadRigidBody2D(node, &props, engine);
            } else if (type == "CharacterBody2D") {
                _loadCharacterBody2D(node, &props, engine);
            } else if (type == "StaticBody2D" || type == "Area2D") {
                _loadStaticBody2D(node, &props, engine);
            } else if (type == "TileMap") {
                _loadTileMap(node, &props);
            } else if (type == "CPUParticles2D") {
                _loadParticles(node, &props);
            } else if (type == "AudioStreamPlayer" || type == "AudioStreamPlayer2D") {
                _loadAudio(node, &props, projectPath);
            }
        }

        // Attach script if present
        if (jNode.contains("script") && !jNode["script"].is_null()) {
            std::string scriptName = jNode["script"].get<std::string>();
            if (!scriptName.empty()) {
                namespace fs2 = std::filesystem;
                // Try <project>/scripts/<name>
                // Editor stores .py scripts; runtime uses .nova (if it exists)
                fs2::path novaSc = fs2::path(projectPath) / "scripts" / scriptName;
                // Swap .py extension to .nova if necessary
                if (novaSc.extension() == ".py") {
                    novaSc.replace_extension(".nova");
                }
                if (auto* sn = dynamic_cast<ScriptNode*>(node)) {
                    if (fs2::exists(novaSc)) {
                        sn->LoadScript(novaSc.string());
                        NOVA_LOG("SceneLoader: attached script '", novaSc.string(),
                                 "' to node '", name, "'");
                    } else {
                        NOVA_LOG("SceneLoader: script not found: ", novaSc.string(),
                                 " (no NovaScript attached)");
                    }
                }
            }
        }

        nodeMap[id] = node;
        NOVA_LOG("  + Node '", name, "' [", type, "] id=", id);
    }

    // ── Pass 2: Build parent-child hierarchy ────────────────────────
    for (const auto& jNode : sceneArr) {
        std::string id = jget<std::string>(jNode, "id", "");
        auto it = nodeMap.find(id);
        if (it == nodeMap.end()) continue;
        Node* node = it->second;

        bool hasParent = jNode.contains("parent") && !jNode["parent"].is_null();
        if (hasParent) {
            std::string parentId = jNode["parent"].get<std::string>();
            auto pit = nodeMap.find(parentId);
            if (pit != nodeMap.end()) {
                pit->second->AddChild(node);
            } else {
                // Orphan — attach to scene root
                engine->GetSceneTree()->AddToRoot(node);
            }
        } else {
            // Top-level node → add directly to root
            engine->GetSceneTree()->AddToRoot(node);
        }
    }

    NOVA_LOG("SceneLoader: scene '", scenePath.filename().string(), "' loaded successfully.");
    return true;
}

} // namespace Nova
