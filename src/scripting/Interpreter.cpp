#include "scripting/Interpreter.h"
#include "scene/Node.h"
#include "core/Engine.h"
#include "core/Logger.h"
#include "input/InputManager.h"
#include <cmath>
#include <sstream>
#include <iostream>

namespace Nova {

// ─── NovaValue Methods ──────────────────────────────────
bool NovaValue::IsTruthy() const {
    if (IsNull()) return false;
    if (IsBool()) return AsBool();
    if (IsNumber()) return AsNumber() != 0.0;
    if (IsString()) return !AsString().empty();
    if (IsArray()) return !AsArray().empty();
    if (IsNode()) return AsNode() != nullptr;
    return false;
}

std::string NovaValue::ToString() const {
    if (IsNull()) return "null";
    if (IsBool()) return AsBool() ? "true" : "false";
    if (IsNumber()) {
        double v = AsNumber();
        if (v == (int)v) return std::to_string((int)v);
        std::ostringstream oss; oss << v; return oss.str();
    }
    if (IsString()) return AsString();
    if (IsArray()) {
        std::string s = "[";
        auto& arr = AsArray();
        for (size_t i = 0; i < arr.size(); i++) {
            if (i > 0) s += ", ";
            s += arr[i].ToString();
        }
        return s + "]";
    }
    if (IsNode()) {
        Node* n = AsNode();
        return n ? "<Node '" + n->GetName() + "'>" : "<Node null>";
    }
    return "<unknown>";
}

// ─── Environment ────────────────────────────────────────
void Environment::Set(const std::string& name, const NovaValue& value) {
    // Walk up scope chain to find existing variable
    Environment* env = this;
    while (env) {
        auto it = env->variables_.find(name);
        if (it != env->variables_.end()) { it->second = value; return; }
        env = env->parent_;
    }
    variables_[name] = value; // Create in current scope
}

NovaValue Environment::Get(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) return it->second;
    if (parent_) return parent_->Get(name);
    return NovaValue::Null();
}

bool Environment::Has(const std::string& name) const {
    if (variables_.count(name)) return true;
    return parent_ ? parent_->Has(name) : false;
}

void Environment::SetLocal(const std::string& name, const NovaValue& value) {
    variables_[name] = value;
}

// ─── Interpreter ────────────────────────────────────────
Interpreter::Interpreter(Engine* engine) : engine_(engine), currentEnv_(&globalEnv_) {
    RegisterEngineAPI();
}

void Interpreter::Execute(Script* script) {
    if (!script) return;
    for (auto& stmt : script->statements) {
        try {
            stmt->Accept(*this);
        } catch (ReturnException&) {
            // Top-level return
        } catch (BreakException&) {
            errors_.push_back("'break' outside of loop");
        } catch (ContinueException&) {
            errors_.push_back("'continue' outside of loop");
        }
    }
}

void Interpreter::ExecuteNode(ASTNode* node) {
    if (node) node->Accept(*this);
}

NovaValue Interpreter::Evaluate(ASTNode* node) {
    if (!node) return NovaValue::Null();
    node->Accept(*this);
    return result_;
}

void Interpreter::RegisterFunction(const std::string& name, NativeFunction fn) {
    NovaFunction nf;
    nf.name = name;
    nf.nativeImpl = std::move(fn);
    nf.isNative = true;
    functions_[name] = std::move(nf);
}

void Interpreter::SetGlobal(const std::string& name, const NovaValue& value) {
    globalEnv_.SetLocal(name, value);
}

NovaValue Interpreter::GetGlobal(const std::string& name) const {
    return globalEnv_.Get(name);
}

bool Interpreter::HasFunction(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

NovaValue Interpreter::CallFunction(const std::string& name, const std::vector<NovaValue>& args) {
    auto fit = functions_.find(name);
    if (fit == functions_.end()) {
        errors_.push_back("Function not found: " + name);
        return NovaValue::Null();
    }
    auto& func = fit->second;
    if (func.isNative) {
        return func.nativeImpl(args);
    } else if (func.body) {
        Environment funcEnv(currentEnv_);
        for (size_t i = 0; i < func.params.size(); i++) {
            funcEnv.SetLocal(func.params[i], i < args.size() ? args[i] : NovaValue::Null());
        }
        auto* prevEnv = currentEnv_;
        currentEnv_ = &funcEnv;
        NovaValue retVal = NovaValue::Null();
        try {
            func.body->Accept(*this);
        } catch (ReturnException& ret) {
            retVal = ret.value;
        }
        currentEnv_ = prevEnv;
        return retVal;
    }
    return NovaValue::Null();
}

void Interpreter::RegisterEngineAPI() {
    // print(...)  
    RegisterFunction("print", [](const std::vector<NovaValue>& args) -> NovaValue {
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) std::cout << " ";
            std::cout << args[i].ToString();
        }
        std::cout << std::endl;
        return NovaValue::Null();
    });

    // str(value)
    RegisterFunction("str", [](const std::vector<NovaValue>& args) -> NovaValue {
        return args.empty() ? NovaValue("") : NovaValue(args[0].ToString());
    });

    // int(value)
    RegisterFunction("int", [](const std::vector<NovaValue>& args) -> NovaValue {
        if (args.empty()) return NovaValue(0.0);
        if (args[0].IsNumber()) return NovaValue((double)(int)args[0].AsNumber());
        if (args[0].IsString()) {
            try { return NovaValue((double)std::stoi(args[0].AsString())); }
            catch (...) { return NovaValue(0.0); }
        }
        return NovaValue(0.0);
    });

    // float(value)
    RegisterFunction("float", [](const std::vector<NovaValue>& args) -> NovaValue {
        if (args.empty()) return NovaValue(0.0);
        if (args[0].IsNumber()) return args[0];
        if (args[0].IsString()) {
            try { return NovaValue(std::stod(args[0].AsString())); }
            catch (...) { return NovaValue(0.0); }
        }
        return NovaValue(0.0);
    });

    // len(array_or_string)
    RegisterFunction("len", [](const std::vector<NovaValue>& args) -> NovaValue {
        if (args.empty()) return NovaValue(0.0);
        if (args[0].IsArray()) return NovaValue((double)args[0].AsArray().size());
        if (args[0].IsString()) return NovaValue((double)args[0].AsString().size());
        return NovaValue(0.0);
    });

    // range(n) or range(start, end) or range(start, end, step)
    RegisterFunction("range", [](const std::vector<NovaValue>& args) -> NovaValue {
        int start = 0, end = 0, step = 1;
        if (args.size() == 1) { end = (int)args[0].AsNumber(); }
        else if (args.size() >= 2) {
            start = (int)args[0].AsNumber();
            end = (int)args[1].AsNumber();
        }
        if (args.size() >= 3) step = (int)args[2].AsNumber();
        if (step == 0) step = 1;
        std::vector<NovaValue> result;
        if (step > 0) for (int i = start; i < end; i += step) result.push_back(NovaValue((double)i));
        else for (int i = start; i > end; i += step) result.push_back(NovaValue((double)i));
        return NovaValue::Array(result);
    });

    // abs, min, max, clamp, sqrt, sin, cos
    RegisterFunction("abs", [](const std::vector<NovaValue>& a) -> NovaValue {
        return a.empty() ? NovaValue(0.0) : NovaValue(std::fabs(a[0].AsNumber()));
    });
    RegisterFunction("min", [](const std::vector<NovaValue>& a) -> NovaValue {
        return a.size() < 2 ? NovaValue(0.0) : NovaValue(std::fmin(a[0].AsNumber(), a[1].AsNumber()));
    });
    RegisterFunction("max", [](const std::vector<NovaValue>& a) -> NovaValue {
        return a.size() < 2 ? NovaValue(0.0) : NovaValue(std::fmax(a[0].AsNumber(), a[1].AsNumber()));
    });
    RegisterFunction("clamp", [](const std::vector<NovaValue>& a) -> NovaValue {
        if (a.size() < 3) return NovaValue(0.0);
        double v = a[0].AsNumber(), lo = a[1].AsNumber(), hi = a[2].AsNumber();
        return NovaValue(std::fmin(std::fmax(v, lo), hi));
    });
    RegisterFunction("sqrt", [](const std::vector<NovaValue>& a) -> NovaValue {
        return a.empty() ? NovaValue(0.0) : NovaValue(std::sqrt(a[0].AsNumber()));
    });
    RegisterFunction("sin", [](const std::vector<NovaValue>& a) -> NovaValue {
        return a.empty() ? NovaValue(0.0) : NovaValue(std::sin(a[0].AsNumber()));
    });
    RegisterFunction("cos", [](const std::vector<NovaValue>& a) -> NovaValue {
        return a.empty() ? NovaValue(0.0) : NovaValue(std::cos(a[0].AsNumber()));
    });

    // typeof(value)
    RegisterFunction("typeof", [](const std::vector<NovaValue>& a) -> NovaValue {
        if (a.empty() || a[0].IsNull()) return NovaValue("null");
        if (a[0].IsBool()) return NovaValue("bool");
        if (a[0].IsNumber()) return NovaValue("number");
        if (a[0].IsString()) return NovaValue("string");
        if (a[0].IsArray()) return NovaValue("array");
        return NovaValue("unknown");
    });
}

// ─── Visitor Implementation ─────────────────────────────
void Interpreter::Visit(NumberLiteral& n) { result_ = NovaValue(n.value); }
void Interpreter::Visit(StringLiteral& n) { result_ = NovaValue(n.value); }
void Interpreter::Visit(BoolLiteral& n) { result_ = NovaValue(n.value); }
void Interpreter::Visit(NullLiteral& n) { result_ = NovaValue::Null(); }

void Interpreter::Visit(Identifier& n) {
    // Check functions first
    auto fit = functions_.find(n.name);
    if (fit != functions_.end()) {
        result_ = NovaValue(n.name); // Return function name as value
        return;
    }
    result_ = currentEnv_->Get(n.name);
}

void Interpreter::Visit(BinaryExpr& n) {
    NovaValue left = Evaluate(n.left.get());
    
    // Short-circuit for and/or
    if (n.op == "and") { result_ = left.IsTruthy() ? Evaluate(n.right.get()) : left; return; }
    if (n.op == "or") { result_ = left.IsTruthy() ? left : Evaluate(n.right.get()); return; }
    
    NovaValue right = Evaluate(n.right.get());

    // String concatenation
    if (n.op == "+" && (left.IsString() || right.IsString())) {
        result_ = NovaValue(left.ToString() + right.ToString());
        return;
    }

    if (left.IsNumber() && right.IsNumber()) {
        double l = left.AsNumber(), r = right.AsNumber();
        if (n.op == "+") result_ = NovaValue(l + r);
        else if (n.op == "-") result_ = NovaValue(l - r);
        else if (n.op == "*") result_ = NovaValue(l * r);
        else if (n.op == "/") result_ = r != 0 ? NovaValue(l / r) : NovaValue(0.0);
        else if (n.op == "%") result_ = r != 0 ? NovaValue(std::fmod(l, r)) : NovaValue(0.0);
        else if (n.op == "**") result_ = NovaValue(std::pow(l, r));
        else if (n.op == "==") result_ = NovaValue(l == r);
        else if (n.op == "!=") result_ = NovaValue(l != r);
        else if (n.op == "<") result_ = NovaValue(l < r);
        else if (n.op == ">") result_ = NovaValue(l > r);
        else if (n.op == "<=") result_ = NovaValue(l <= r);
        else if (n.op == ">=") result_ = NovaValue(l >= r);
    } else if (n.op == "==") {
        result_ = NovaValue(left.ToString() == right.ToString());
    } else if (n.op == "!=") {
        result_ = NovaValue(left.ToString() != right.ToString());
    }
}

void Interpreter::Visit(UnaryExpr& n) {
    NovaValue val = Evaluate(n.operand.get());
    if (n.op == "-" && val.IsNumber()) result_ = NovaValue(-val.AsNumber());
    else if (n.op == "not") result_ = NovaValue(!val.IsTruthy());
    else result_ = val;
}

void Interpreter::Visit(AssignExpr& n) {
    NovaValue val = Evaluate(n.value.get());
    if (auto* ident = dynamic_cast<Identifier*>(n.target.get())) {
        if (n.op == "=") currentEnv_->Set(ident->name, val);
        else {
            NovaValue cur = currentEnv_->Get(ident->name);
            if (cur.IsNumber() && val.IsNumber()) {
                double c = cur.AsNumber(), v = val.AsNumber();
                if (n.op == "+=") currentEnv_->Set(ident->name, NovaValue(c + v));
                else if (n.op == "-=") currentEnv_->Set(ident->name, NovaValue(c - v));
                else if (n.op == "*=") currentEnv_->Set(ident->name, NovaValue(c * v));
                else if (n.op == "/=") currentEnv_->Set(ident->name, v != 0 ? NovaValue(c / v) : NovaValue(0.0));
            } else if (n.op == "+=" && cur.IsString()) {
                currentEnv_->Set(ident->name, NovaValue(cur.AsString() + val.ToString()));
            }
        }
        result_ = currentEnv_->Get(ident->name);
    }
}

void Interpreter::Visit(CallExpr& n) {
    std::vector<NovaValue> args;
    for (auto& arg : n.arguments) args.push_back(Evaluate(arg.get()));

    // Check if callee is a member call on a Node object (e.g. node.move(10, 20))
    if (auto* mem = dynamic_cast<MemberExpr*>(n.callee.get())) {
        NovaValue obj = Evaluate(mem->object.get());
        if (obj.IsNode()) {
            Node* targetNode = obj.AsNode();
            std::string method = mem->member;
            if (targetNode) {
                if (method == "get_position_x") { result_ = NovaValue((double)targetNode->GetPosition().x); return; }
                if (method == "get_position_y") { result_ = NovaValue((double)targetNode->GetPosition().y); return; }
                if (method == "set_position_x" && !args.empty()) {
                    targetNode->SetPosition({(float)args[0].AsNumber(), targetNode->GetPosition().y});
                    result_ = NovaValue::Null(); return;
                }
                if (method == "set_position_y" && !args.empty()) {
                    targetNode->SetPosition({targetNode->GetPosition().x, (float)args[0].AsNumber()});
                    result_ = NovaValue::Null(); return;
                }
                if (method == "get_rotation") { result_ = NovaValue((double)targetNode->GetRotation()); return; }
                if (method == "set_rotation" && !args.empty()) {
                    targetNode->SetRotation((float)args[0].AsNumber());
                    result_ = NovaValue::Null(); return;
                }
                if (method == "move" || method == "translate") {
                    float dx = args.size() > 0 ? (float)args[0].AsNumber() : 0.0f;
                    float dy = args.size() > 1 ? (float)args[1].AsNumber() : 0.0f;
                    targetNode->SetPosition(targetNode->GetPosition() + Vec2(dx, dy));
                    result_ = NovaValue::Null(); return;
                }
                if (method == "get_child") {
                    if (!args.empty() && args[0].IsString()) result_ = NovaValue(targetNode->FindChild(args[0].AsString(), false));
                    else if (!args.empty() && args[0].IsNumber()) result_ = NovaValue(targetNode->GetChild((int)args[0].AsNumber()));
                    else result_ = NovaValue::Null();
                    return;
                }
                if (method == "get_node" && !args.empty() && args[0].IsString()) {
                    result_ = NovaValue(targetNode->FindChild(args[0].AsString(), true));
                    return;
                }
            }
        }
    }

    NovaValue callee = Evaluate(n.callee.get());

    std::string funcName;
    if (callee.IsString()) funcName = callee.AsString();
    if (auto* ident = dynamic_cast<Identifier*>(n.callee.get())) funcName = ident->name;

    auto fit = functions_.find(funcName);
    if (fit != functions_.end()) {
        auto& func = fit->second;
        if (func.isNative) {
            result_ = func.nativeImpl(args);
        } else if (func.body) {
            Environment funcEnv(currentEnv_);
            for (size_t i = 0; i < func.params.size(); i++) {
                funcEnv.SetLocal(func.params[i], i < args.size() ? args[i] : NovaValue::Null());
            }
            auto* prevEnv = currentEnv_;
            currentEnv_ = &funcEnv;
            try {
                func.body->Accept(*this);
                result_ = NovaValue::Null();
            } catch (ReturnException& ret) {
                result_ = ret.value;
            }
            currentEnv_ = prevEnv;
        }
    } else {
        errors_.push_back("Unknown function: " + funcName);
        result_ = NovaValue::Null();
    }
}

void Interpreter::Visit(MemberExpr& n) {
    NovaValue obj = Evaluate(n.object.get());
    if (obj.IsArray()) {
        if (n.member == "size" || n.member == "length") {
            result_ = NovaValue((double)obj.AsArray().size());
        } else if (n.member == "empty") {
            result_ = NovaValue(obj.AsArray().empty());
        }
    } else if (obj.IsString()) {
        if (n.member == "length") result_ = NovaValue((double)obj.AsString().size());
        else if (n.member == "empty") result_ = NovaValue(obj.AsString().empty());
    }
}

void Interpreter::Visit(IndexExpr& n) {
    NovaValue obj = Evaluate(n.object.get());
    NovaValue idx = Evaluate(n.index.get());
    if (obj.IsArray() && idx.IsNumber()) {
        auto& arr = obj.AsArray();
        int i = (int)idx.AsNumber();
        if (i >= 0 && i < (int)arr.size()) result_ = arr[i];
        else result_ = NovaValue::Null();
    } else if (obj.IsString() && idx.IsNumber()) {
        int i = (int)idx.AsNumber();
        auto& s = obj.AsString();
        if (i >= 0 && i < (int)s.size()) result_ = NovaValue(std::string(1, s[i]));
        else result_ = NovaValue::Null();
    } else result_ = NovaValue::Null();
}

void Interpreter::Visit(ArrayLiteral& n) {
    std::vector<NovaValue> elems;
    for (auto& e : n.elements) elems.push_back(Evaluate(e.get()));
    result_ = NovaValue::Array(elems);
}

void Interpreter::Visit(VarDecl& n) {
    NovaValue val = n.initializer ? Evaluate(n.initializer.get()) : NovaValue::Null();
    currentEnv_->SetLocal(n.name, val);
    result_ = val;
}

void Interpreter::Visit(FuncDecl& n) {
    NovaFunction func;
    func.name = n.name;
    func.params = n.params;
    func.body = n.body.get();
    func.isNative = false;
    functions_[n.name] = std::move(func);
}

void Interpreter::Visit(ClassDecl& n) {
    // Basic class support - register constructor and methods
    for (auto& member : n.members) {
        if (auto* funcDecl = dynamic_cast<FuncDecl*>(member.get())) {
            std::string qualName = n.name + "." + funcDecl->name;
            NovaFunction func;
            func.name = qualName;
            func.params = funcDecl->params;
            func.body = funcDecl->body.get();
            functions_[qualName] = std::move(func);
        }
    }
}

void Interpreter::Visit(ReturnStmt& n) {
    NovaValue val = n.value ? Evaluate(n.value.get()) : NovaValue::Null();
    throw ReturnException{val};
}

void Interpreter::Visit(IfStmt& n) {
    NovaValue cond = Evaluate(n.condition.get());
    if (cond.IsTruthy()) {
        n.thenBranch->Accept(*this);
    } else {
        bool handled = false;
        for (auto& [elifCond, elifBody] : n.elifBranches) {
            if (Evaluate(elifCond.get()).IsTruthy()) {
                elifBody->Accept(*this);
                handled = true;
                break;
            }
        }
        if (!handled && n.elseBranch) {
            n.elseBranch->Accept(*this);
        }
    }
}

void Interpreter::Visit(WhileStmt& n) {
    int safety = 0;
    while (Evaluate(n.condition.get()).IsTruthy() && safety++ < 1000000) {
        try {
            n.body->Accept(*this);
        } catch (BreakException&) { break; }
        catch (ContinueException&) { continue; }
    }
}

void Interpreter::Visit(ForStmt& n) {
    NovaValue iterable = Evaluate(n.iterable.get());
    if (iterable.IsArray()) {
        for (auto& elem : iterable.AsArray()) {
            currentEnv_->SetLocal(n.varName, elem);
            try {
                n.body->Accept(*this);
            } catch (BreakException&) { break; }
            catch (ContinueException&) { continue; }
        }
    }
}

void Interpreter::Visit(BreakStmt& n) { (void)n; throw BreakException{}; }
void Interpreter::Visit(ContinueStmt& n) { (void)n; throw ContinueException{}; }
void Interpreter::Visit(PassStmt& n) { (void)n; }

void Interpreter::Visit(ExprStatement& n) { Evaluate(n.expression.get()); }

void Interpreter::Visit(Block& n) {
    Environment blockEnv(currentEnv_);
    auto* prev = currentEnv_;
    currentEnv_ = &blockEnv;
    for (auto& stmt : n.statements) {
        stmt->Accept(*this);
    }
    currentEnv_ = prev;
}

void Interpreter::Visit(ExtendsStmt& n) {
    // Handled at script level
    (void)n;
}

void Interpreter::ExecuteBlock(Block& block, Environment& env) {
    auto* prev = currentEnv_;
    currentEnv_ = &env;
    for (auto& stmt : block.statements) {
        stmt->Accept(*this);
    }
    currentEnv_ = prev;
}

} // namespace Nova
