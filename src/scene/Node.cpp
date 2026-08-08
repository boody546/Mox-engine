#include "scene/Node.h"
#include <algorithm>

namespace Nova {

Node::Node(const std::string& name) : name_(name) {}

Node::~Node() {
    for (auto* child : children_) {
        child->parent_ = nullptr;
        delete child;
    }
    children_.clear();
}

void Node::AddChild(Node* child) {
    if (!child || child == this) return;
    if (child->parent_) child->parent_->RemoveChild(child);
    child->parent_ = this;
    children_.push_back(child);
    if (!child->ready_) {
        child->_Ready();
        child->ready_ = true;
    }
}

void Node::RemoveChild(Node* child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

Node* Node::GetChild(int index) const {
    if (index < 0 || index >= (int)children_.size()) return nullptr;
    return children_[index];
}

Node* Node::FindChild(const std::string& name, bool recursive) const {
    for (auto* child : children_) {
        if (child->name_ == name) return child;
        if (recursive) {
            Node* found = child->FindChild(name, true);
            if (found) return found;
        }
    }
    return nullptr;
}

void Node::SetParent(Node* newParent) {
    if (parent_) parent_->RemoveChild(this);
    if (newParent) newParent->AddChild(this);
}

Vec2 Node::GetGlobalPosition() const {
    Vec2 pos = transform_.position;
    if (parent_) pos = pos + parent_->GetGlobalPosition();
    return pos;
}

float Node::GetGlobalRotation() const {
    float rot = transform_.rotation;
    if (parent_) rot += parent_->GetGlobalRotation();
    return rot;
}

void Node::QueueFree() { queuedForDeletion_ = true; }

} // namespace Nova
