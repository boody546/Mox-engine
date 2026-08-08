#pragma once
// ═══════════════════════════════════════════════════════════════════
//  NovaScript — Interpreter
//  Tree-walking interpreter for NovaScript AST
// ═══════════════════════════════════════════════════════════════════

#include "scripting/AST.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>
#include <memory>

namespace Nova {

class Engine;
class Node;

// NovaScript runtime value types
using NovaNull = std::monostate;
using NovaArray = std::vector<struct NovaValue>;

struct NovaValue {
    std::variant<NovaNull, double, bool, std::string, std::shared_ptr<NovaArray>, Node*> data;

    NovaValue() : data(NovaNull{}) {}
    NovaValue(double v) : data(v) {}
    NovaValue(int v) : data((double)v) {}
    NovaValue(bool v) : data(v) {}
    NovaValue(const std::string& v) : data(v) {}
    NovaValue(const char* v) : data(std::string(v)) {}
    NovaValue(Node* n) : data(n) {}

    static NovaValue Null() { return NovaValue(); }
    static NovaValue Array(const std::vector<NovaValue>& elements = {}) {
        NovaValue v;
        v.data = std::make_shared<NovaArray>(elements);
        return v;
    }

    bool IsNull() const { return std::holds_alternative<NovaNull>(data); }
    bool IsNumber() const { return std::holds_alternative<double>(data); }
    bool IsBool() const { return std::holds_alternative<bool>(data); }
    bool IsString() const { return std::holds_alternative<std::string>(data); }
    bool IsArray() const { return std::holds_alternative<std::shared_ptr<NovaArray>>(data); }
    bool IsNode() const { return std::holds_alternative<Node*>(data); }

    double AsNumber() const { return std::get<double>(data); }
    bool AsBool() const { return std::get<bool>(data); }
    const std::string& AsString() const { return std::get<std::string>(data); }
    NovaArray& AsArray() const { return *std::get<std::shared_ptr<NovaArray>>(data); }
    Node* AsNode() const { return std::get<Node*>(data); }

    bool IsTruthy() const;
    std::string ToString() const;
};

// Function type that can be called from NovaScript
using NativeFunction = std::function<NovaValue(const std::vector<NovaValue>&)>;

struct NovaFunction {
    std::string name;
    std::vector<std::string> params;
    ASTNode* body = nullptr; // Block node
    NativeFunction nativeImpl;
    bool isNative = false;
};

// Scope / Environment
class Environment {
public:
    Environment(Environment* parent = nullptr) : parent_(parent) {}

    void Set(const std::string& name, const NovaValue& value);
    NovaValue Get(const std::string& name) const;
    bool Has(const std::string& name) const;
    void SetLocal(const std::string& name, const NovaValue& value);
    Environment* GetParent() const { return parent_; }

private:
    std::unordered_map<std::string, NovaValue> variables_;
    Environment* parent_ = nullptr;
};

// Control flow exceptions
struct ReturnException { NovaValue value; };
struct BreakException {};
struct ContinueException {};

class Interpreter : public ASTVisitor {
public:
    explicit Interpreter(Engine* engine = nullptr);
    ~Interpreter() = default;

    // Execute a parsed script
    void Execute(Script* script);

    // Execute a single statement
    void ExecuteNode(ASTNode* node);

    // Register native function
    void RegisterFunction(const std::string& name, NativeFunction fn);

    // Register engine API
    void RegisterEngineAPI();

    // Call a script function by name
    NovaValue CallFunction(const std::string& name, const std::vector<NovaValue>& args = {});
    bool HasFunction(const std::string& name) const;

    // Get result of last expression
    NovaValue GetResult() const { return result_; }

    // Direct variable access
    void SetGlobal(const std::string& name, const NovaValue& value);
    NovaValue GetGlobal(const std::string& name) const;
    Environment& GetGlobalEnv() { return globalEnv_; }

    // Error handling
    bool HasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& GetErrors() const { return errors_; }

    // Visitor implementation
    void Visit(NumberLiteral& n) override;
    void Visit(StringLiteral& n) override;
    void Visit(BoolLiteral& n) override;
    void Visit(NullLiteral& n) override;
    void Visit(Identifier& n) override;
    void Visit(BinaryExpr& n) override;
    void Visit(UnaryExpr& n) override;
    void Visit(AssignExpr& n) override;
    void Visit(CallExpr& n) override;
    void Visit(MemberExpr& n) override;
    void Visit(IndexExpr& n) override;
    void Visit(ArrayLiteral& n) override;
    void Visit(VarDecl& n) override;
    void Visit(FuncDecl& n) override;
    void Visit(ClassDecl& n) override;
    void Visit(ReturnStmt& n) override;
    void Visit(IfStmt& n) override;
    void Visit(WhileStmt& n) override;
    void Visit(ForStmt& n) override;
    void Visit(BreakStmt& n) override;
    void Visit(ContinueStmt& n) override;
    void Visit(PassStmt& n) override;
    void Visit(ExprStatement& n) override;
    void Visit(Block& n) override;
    void Visit(ExtendsStmt& n) override;

private:
    NovaValue Evaluate(ASTNode* node);
    void ExecuteBlock(Block& block, Environment& env);

    Engine* engine_;
    Environment globalEnv_;
    Environment* currentEnv_;
    NovaValue result_;
    std::unordered_map<std::string, NovaFunction> functions_;
    std::vector<std::string> errors_;
};

} // namespace Nova
