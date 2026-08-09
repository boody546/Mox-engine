#pragma once
// ═══════════════════════════════════════════════════════════════════
//  Mox Engine — SceneLoader
//  Deserializes a scenes/main.json exported from HUB/editor.py
//  into the live SceneTree.
//  Format: { "version":"4.0", "scene": [ { "id","name","type","parent","props":{} }, … ] }
// ═══════════════════════════════════════════════════════════════════

#include <string>
#include <unordered_map>
#include <vector>

namespace Nova {

class Engine;
class Node;
class SceneTree;

class SceneLoader {
public:
    // ── Main entry point ────────────────────────────────────────────
    // projectPath  : absolute path to the user's project directory
    // relScenePath : relative path inside the project, e.g. "scenes/main.json"
    // engine       : running Engine instance (provides renderer, physics, resources)
    // Returns true on success, false on parse/IO error.
    static bool LoadFromFile(const std::string& projectPath,
                             const std::string& relScenePath,
                             Engine* engine);

private:
    // ── Per-type instantiation helpers ──────────────────────────────
    static Node* _createNode    (const std::string& type, const std::string& name);
    static void  _applyCommon   (Node* node, const void* jProps);
    static void  _loadSprite2D         (Node* node, const void* jProps, const std::string& projectPath, Engine* engine);
    static void  _loadLight2D          (Node* node, const void* jProps);
    static void  _loadCamera2D         (Node* node, const void* jProps, Engine* engine);
    static void  _loadRigidBody2D      (Node* node, const void* jProps, Engine* engine);
    static void  _loadCharacterBody2D  (Node* node, const void* jProps, Engine* engine);
    static void  _loadStaticBody2D     (Node* node, const void* jProps, Engine* engine);
    static void  _loadTileMap          (Node* node, const void* jProps);
    static void  _loadParticles        (Node* node, const void* jProps);
    static void  _loadAudio            (Node* node, const void* jProps, const std::string& projectPath);

    // ── Utility ─────────────────────────────────────────────────────
    // Parse "#rrggbb" or "#rrggbbaa" hex colour string → Color
    static Nova::Color _hexToColor(const std::string& hex);
};

} // namespace Nova
