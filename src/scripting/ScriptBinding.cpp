#include "scripting/ScriptBinding.h"
#include "core/Logger.h"

namespace Nova {

void ScriptBinding::BindNode(Interpreter* interpreter, Node* node) {
    if (!interpreter || !node) return;

    // Register 'self' and 'this' global variables
    interpreter->SetGlobal("self", NovaValue(node));
    interpreter->SetGlobal("this", NovaValue(node));

    // get_position_x()
    interpreter->RegisterFunction("get_position_x", [node](const std::vector<NovaValue>&) -> NovaValue {
        return NovaValue((double)node->GetPosition().x);
    });

    // set_position_x(val)
    interpreter->RegisterFunction("set_position_x", [node](const std::vector<NovaValue>& args) -> NovaValue {
        if (!args.empty() && args[0].IsNumber()) {
            Vec2 pos = node->GetPosition();
            pos.x = (float)args[0].AsNumber();
            node->SetPosition(pos);
        }
        return NovaValue::Null();
    });

    // get_position_y()
    interpreter->RegisterFunction("get_position_y", [node](const std::vector<NovaValue>&) -> NovaValue {
        return NovaValue((double)node->GetPosition().y);
    });

    // set_position_y(val)
    interpreter->RegisterFunction("set_position_y", [node](const std::vector<NovaValue>& args) -> NovaValue {
        if (!args.empty() && args[0].IsNumber()) {
            Vec2 pos = node->GetPosition();
            pos.y = (float)args[0].AsNumber();
            node->SetPosition(pos);
        }
        return NovaValue::Null();
    });

    // get_rotation()
    interpreter->RegisterFunction("get_rotation", [node](const std::vector<NovaValue>&) -> NovaValue {
        return NovaValue((double)node->GetRotation());
    });

    // set_rotation(val)
    interpreter->RegisterFunction("set_rotation", [node](const std::vector<NovaValue>& args) -> NovaValue {
        if (!args.empty() && args[0].IsNumber()) {
            node->SetRotation((float)args[0].AsNumber());
        }
        return NovaValue::Null();
    });

    // get_scale_x()
    interpreter->RegisterFunction("get_scale_x", [node](const std::vector<NovaValue>&) -> NovaValue {
        return NovaValue((double)node->GetScale().x);
    });

    // set_scale_x(val)
    interpreter->RegisterFunction("set_scale_x", [node](const std::vector<NovaValue>& args) -> NovaValue {
        if (!args.empty() && args[0].IsNumber()) {
            Vec2 scale = node->GetScale();
            scale.x = (float)args[0].AsNumber();
            node->SetScale(scale);
        }
        return NovaValue::Null();
    });

    // get_scale_y()
    interpreter->RegisterFunction("get_scale_y", [node](const std::vector<NovaValue>&) -> NovaValue {
        return NovaValue((double)node->GetScale().y);
    });

    // set_scale_y(val)
    interpreter->RegisterFunction("set_scale_y", [node](const std::vector<NovaValue>& args) -> NovaValue {
        if (!args.empty() && args[0].IsNumber()) {
            Vec2 scale = node->GetScale();
            scale.y = (float)args[0].AsNumber();
            node->SetScale(scale);
        }
        return NovaValue::Null();
    });

    // move(dx, dy) / translate(dx, dy)
    auto moveFn = [node](const std::vector<NovaValue>& args) -> NovaValue {
        float dx = args.size() > 0 && args[0].IsNumber() ? (float)args[0].AsNumber() : 0.0f;
        float dy = args.size() > 1 && args[1].IsNumber() ? (float)args[1].AsNumber() : 0.0f;
        Vec2 pos = node->GetPosition();
        node->SetPosition(pos + Vec2(dx, dy));
        return NovaValue::Null();
    };
    interpreter->RegisterFunction("move", moveFn);
    interpreter->RegisterFunction("translate", moveFn);

    // get_child(name_or_index)
    interpreter->RegisterFunction("get_child", [node](const std::vector<NovaValue>& args) -> NovaValue {
        if (args.empty()) return NovaValue::Null();
        if (args[0].IsString()) {
            Node* child = node->FindChild(args[0].AsString(), false);
            return child ? NovaValue(child) : NovaValue::Null();
        } else if (args[0].IsNumber()) {
            Node* child = node->GetChild((int)args[0].AsNumber());
            return child ? NovaValue(child) : NovaValue::Null();
        }
        return NovaValue::Null();
    });

    // get_node(path)
    interpreter->RegisterFunction("get_node", [node](const std::vector<NovaValue>& args) -> NovaValue {
        if (args.empty() || !args[0].IsString()) return NovaValue::Null();
        Node* target = node->FindChild(args[0].AsString(), true);
        return target ? NovaValue(target) : NovaValue::Null();
    });

    // get_name()
    interpreter->RegisterFunction("get_name", [node](const std::vector<NovaValue>&) -> NovaValue {
        return NovaValue(node->GetName());
    });
}

} // namespace Nova
