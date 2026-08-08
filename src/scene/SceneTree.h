#pragma once
#include "scene/Node.h"
#include <vector>
#include <functional>
#include <string>

namespace Nova {

class Engine;
class Renderer2D;

class SceneTree {
public:
    explicit SceneTree(Engine* engine);
    ~SceneTree();

    void Init();
    void Update(float dt);
    void Render(Renderer2D* renderer);

    // Root node
    Node* GetRoot() const { return root_; }

    // Scene management
    void SetCurrentScene(Node* scene);
    Node* GetCurrentScene() const { return currentScene_; }

    // Helpers
    void AddToRoot(Node* node);
    Node* FindNode(const std::string& name) const;

    // Process queued deletions
    void ProcessDeletions();

    // Engine reference
    Engine* GetEngine() const { return engine_; }

    // Node groups
    void AddToGroup(Node* node, const std::string& group);
    void RemoveFromGroup(Node* node, const std::string& group);
    std::vector<Node*> GetNodesInGroup(const std::string& group) const;

private:
    void UpdateNode(Node* node, float dt);
    void RenderNode(Node* node, Renderer2D* renderer);
    void CollectDeletions(Node* node, std::vector<Node*>& toDelete);

    Engine* engine_;
    Node* root_ = nullptr;
    Node* currentScene_ = nullptr;

    // Groups: group name -> list of nodes
    std::unordered_map<std::string, std::vector<Node*>> groups_;
};

} // namespace Nova
