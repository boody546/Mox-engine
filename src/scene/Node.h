#pragma once
#include "core/Math.h"
#include "core/Logger.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Nova {

class Renderer2D;
class Engine;

class Node {
public:
    Node(const std::string& name = "Node");
    virtual ~Node();

    // Lifecycle (override in derived classes)
    virtual void _Ready() {}
    virtual void _Update(float dt) {}
    virtual void _Draw(Renderer2D* renderer) {}
    virtual void _OnDestroy() {}

    // Tree operations
    void AddChild(Node* child);
    void RemoveChild(Node* child);
    Node* GetChild(int index) const;
    Node* FindChild(const std::string& name, bool recursive = true) const;
    int GetChildCount() const { return (int)children_.size(); }
    const std::vector<Node*>& GetChildren() const { return children_; }

    // Parent
    Node* GetParent() const { return parent_; }
    void SetParent(Node* parent);

    // Properties
    const std::string& GetName() const { return name_; }
    void SetName(const std::string& n) { name_ = n; }
    bool IsVisible() const { return visible_; }
    void SetVisible(bool v) { visible_ = v; }
    bool IsActive() const { return active_; }
    void SetActive(bool a) { active_ = a; }
    int GetZIndex() const { return zIndex_; }
    void SetZIndex(int z) { zIndex_ = z; }

    // Transform
    Transform2D& GetTransform() { return transform_; }
    const Transform2D& GetTransform() const { return transform_; }
    Vec2 GetPosition() const { return transform_.position; }
    void SetPosition(const Vec2& pos) { transform_.position = pos; }
    float GetRotation() const { return transform_.rotation; }
    void SetRotation(float r) { transform_.rotation = r; }
    Vec2 GetScale() const { return transform_.scale; }
    void SetScale(const Vec2& s) { transform_.scale = s; }

    // Global transform
    Vec2 GetGlobalPosition() const;
    float GetGlobalRotation() const;

    // Queue for deletion at end of frame
    void QueueFree();
    bool IsQueuedForDeletion() const { return queuedForDeletion_; }

    // Type identification
    virtual std::string GetType() const { return "Node"; }

protected:
    std::string name_;
    Transform2D transform_;
    Node* parent_ = nullptr;
    std::vector<Node*> children_;
    bool visible_ = true;
    bool active_ = true;
    int zIndex_ = 0;
    bool queuedForDeletion_ = false;
    bool ready_ = false;

    friend class SceneTree;
};

} // namespace Nova
