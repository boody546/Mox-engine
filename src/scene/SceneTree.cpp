#include "scene/SceneTree.h"
#include "core/Engine.h"
#include "rendering/Renderer2D.h"
#include <algorithm>

namespace Nova {

SceneTree::SceneTree(Engine* engine) : engine_(engine) {
    root_ = new Node("Root");
}

SceneTree::~SceneTree() {
    delete root_;
    root_ = nullptr;
}

void SceneTree::Init() {
    if (root_ && !root_->ready_) {
        root_->_Ready();
        root_->ready_ = true;
    }
}

void SceneTree::Update(float dt) {
    if (root_) UpdateNode(root_, dt);
    ProcessDeletions();
}

void SceneTree::UpdateNode(Node* node, float dt) {
    if (!node || !node->active_) return;
    node->_Update(dt);
    // Copy children list in case it's modified during update
    auto children = node->GetChildren();
    for (auto* child : children) {
        UpdateNode(child, dt);
    }
}

void SceneTree::Render(Renderer2D* renderer) {
    if (root_) RenderNode(root_, renderer);
}

void SceneTree::RenderNode(Node* node, Renderer2D* renderer) {
    if (!node || !node->visible_ || !node->active_) return;

    // Sort children by z-index for proper layering
    auto children = node->GetChildren();
    std::sort(children.begin(), children.end(), [](const Node* a, const Node* b) {
        return a->GetZIndex() < b->GetZIndex();
    });

    // Render children with negative z-index first
    for (auto* child : children) {
        if (child->GetZIndex() < 0) RenderNode(child, renderer);
    }

    // Render this node
    node->_Draw(renderer);

    // Render children with non-negative z-index
    for (auto* child : children) {
        if (child->GetZIndex() >= 0) RenderNode(child, renderer);
    }
}

void SceneTree::SetCurrentScene(Node* scene) {
    if (currentScene_) {
        root_->RemoveChild(currentScene_);
        delete currentScene_;
    }
    currentScene_ = scene;
    if (scene) root_->AddChild(scene);
}

void SceneTree::AddToRoot(Node* node) {
    if (root_ && node) root_->AddChild(node);
}

Node* SceneTree::FindNode(const std::string& name) const {
    return root_ ? root_->FindChild(name, true) : nullptr;
}

void SceneTree::ProcessDeletions() {
    std::vector<Node*> toDelete;
    if (root_) CollectDeletions(root_, toDelete);

    for (auto* node : toDelete) {
        if (node->parent_) {
            node->parent_->RemoveChild(node);
        }
        node->_OnDestroy();
        // Remove from groups
        for (auto& [groupName, members] : groups_) {
            members.erase(std::remove(members.begin(), members.end(), node), members.end());
        }
        delete node;
    }
}

void SceneTree::CollectDeletions(Node* node, std::vector<Node*>& toDelete) {
    for (auto* child : node->GetChildren()) {
        CollectDeletions(child, toDelete);
    }
    if (node->queuedForDeletion_ && node != root_) {
        toDelete.push_back(node);
    }
}

void SceneTree::AddToGroup(Node* node, const std::string& group) {
    auto& members = groups_[group];
    if (std::find(members.begin(), members.end(), node) == members.end()) {
        members.push_back(node);
    }
}

void SceneTree::RemoveFromGroup(Node* node, const std::string& group) {
    auto it = groups_.find(group);
    if (it != groups_.end()) {
        it->second.erase(std::remove(it->second.begin(), it->second.end(), node),
                          it->second.end());
    }
}

std::vector<Node*> SceneTree::GetNodesInGroup(const std::string& group) const {
    auto it = groups_.find(group);
    return it != groups_.end() ? it->second : std::vector<Node*>{};
}

} // namespace Nova
