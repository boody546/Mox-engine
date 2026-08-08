#pragma once
// ═══════════════════════════════════════════════════════════════════
//  Nova2D Engine — ScriptBinding
//  Binds C++ Node and Transform properties/methods to NovaScript
// ═══════════════════════════════════════════════════════════════════

#include "scripting/Interpreter.h"
#include "scene/Node.h"

namespace Nova {

class ScriptBinding {
public:
    // Binds a C++ Node instance to the given Interpreter environment.
    // Exposes transform getters/setters, translation, scene tree navigation,
    // and registers 'self' / 'this' variable.
    static void BindNode(Interpreter* interpreter, Node* node);
};

} // namespace Nova
