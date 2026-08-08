#pragma once
// ═══════════════════════════════════════════════════════════════════
//  Nova2D Engine — ScriptNode
//  A Node that executes a NovaScript file and exposes _ready and _process
// ═══════════════════════════════════════════════════════════════════

#include "scene/Node.h"
#include "scripting/Interpreter.h"
#include "scripting/AST.h"
#include <string>
#include <memory>

namespace Nova {

class ScriptNode : public Node {
public:
    ScriptNode(const std::string& name = "ScriptNode");
    virtual ~ScriptNode() = default;

    // Load a NovaScript file from disk
    bool LoadScript(const std::string& filePath);

    // Load NovaScript code directly from a string
    bool LoadScriptSource(const std::string& code, const std::string& scriptName = "inline");

    // Override Node lifecycle methods
    void _Ready() override;
    void _Update(float dt) override;

    // Accessors
    Interpreter& GetInterpreter() { return interpreter_; }
    const std::string& GetScriptPath() const { return scriptPath_; }
    bool IsScriptLoaded() const { return isLoaded_; }

    std::string GetType() const override { return "ScriptNode"; }

private:
    std::string scriptPath_;
    Interpreter interpreter_;
    std::unique_ptr<Script> scriptAST_;
    bool isLoaded_ = false;
    bool hasReadyFunc_ = false;
    bool hasProcessFunc_ = false;
};

} // namespace Nova
