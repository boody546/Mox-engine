#include "scene/ScriptNode.h"
#include "scripting/ScriptBinding.h"
#include "scripting/Lexer.h"
#include "scripting/Parser.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>

namespace Nova {

ScriptNode::ScriptNode(const std::string& name) : Node(name) {}

bool ScriptNode::LoadScript(const std::string& filePath) {
    scriptPath_ = filePath;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        NOVA_ERROR("ScriptNode: Could not open script file: " + filePath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return LoadScriptSource(buffer.str(), filePath);
}

bool ScriptNode::LoadScriptSource(const std::string& code, const std::string& scriptName) {
    try {
        Lexer lexer(code, scriptName);
        auto tokens = lexer.Tokenize();
        if (lexer.HasErrors()) {
            for (const auto& err : lexer.GetErrors()) {
                NOVA_ERROR("[Lexer Error] " + err);
            }
            return false;
        }

        Parser parser(tokens, scriptName);
        scriptAST_ = parser.Parse();
        if (parser.HasErrors()) {
            for (const auto& err : parser.GetErrors()) {
                NOVA_ERROR("[Parser Error] " + err);
            }
            return false;
        }

        isLoaded_ = true;
        NOVA_LOG("ScriptNode '" + GetName() + "': Loaded script successfully from " + scriptName);
        return true;
    } catch (const std::exception& e) {
        NOVA_ERROR("ScriptNode Exception while loading script: " + std::string(e.what()));
        return false;
    }
}

void ScriptNode::_Ready() {
    if (!isLoaded_ || !scriptAST_) return;

    // Bind this C++ Node to the interpreter environment
    ScriptBinding::BindNode(&interpreter_, this);

    // Execute top-level declarations and statements
    interpreter_.Execute(scriptAST_.get());

    if (interpreter_.HasErrors()) {
        for (const auto& err : interpreter_.GetErrors()) {
            NOVA_ERROR("[Script Runtime Error] " + err);
        }
    }

    // Cache presence of _ready and _process functions
    hasReadyFunc_ = interpreter_.HasFunction("_ready");
    hasProcessFunc_ = interpreter_.HasFunction("_process");

    // Call _ready() if defined in NovaScript
    if (hasReadyFunc_) {
        interpreter_.CallFunction("_ready");
    }
}

void ScriptNode::_Update(float dt) {
    if (isLoaded_ && hasProcessFunc_) {
        interpreter_.CallFunction("_process", { NovaValue((double)dt) });
    }
}

} // namespace Nova
