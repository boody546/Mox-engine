#pragma once
// ═══════════════════════════════════════════════════════════════════
//  NovaScript — Abstract Syntax Tree
// ═══════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <memory>

namespace Nova {

// Forward declare all AST node types
struct ASTNode;
struct NumberLiteral;
struct StringLiteral;
struct BoolLiteral;
struct NullLiteral;
struct Identifier;
struct BinaryExpr;
struct UnaryExpr;
struct AssignExpr;
struct CallExpr;
struct MemberExpr;
struct IndexExpr;
struct ArrayLiteral;
struct DictLiteral;

struct VarDecl;
struct FuncDecl;
struct ClassDecl;
struct ReturnStmt;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct BreakStmt;
struct ContinueStmt;
struct PassStmt;
struct ExprStatement;
struct Block;
struct ExtendsStmt;
struct SignalDecl;
struct EmitStmt;

// Visitor pattern for AST traversal
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void Visit(NumberLiteral& n) = 0;
    virtual void Visit(StringLiteral& n) = 0;
    virtual void Visit(BoolLiteral& n) = 0;
    virtual void Visit(NullLiteral& n) = 0;
    virtual void Visit(Identifier& n) = 0;
    virtual void Visit(BinaryExpr& n) = 0;
    virtual void Visit(UnaryExpr& n) = 0;
    virtual void Visit(AssignExpr& n) = 0;
    virtual void Visit(CallExpr& n) = 0;
    virtual void Visit(MemberExpr& n) = 0;
    virtual void Visit(IndexExpr& n) = 0;
    virtual void Visit(ArrayLiteral& n) = 0;
    virtual void Visit(VarDecl& n) = 0;
    virtual void Visit(FuncDecl& n) = 0;
    virtual void Visit(ClassDecl& n) = 0;
    virtual void Visit(ReturnStmt& n) = 0;
    virtual void Visit(IfStmt& n) = 0;
    virtual void Visit(WhileStmt& n) = 0;
    virtual void Visit(ForStmt& n) = 0;
    virtual void Visit(BreakStmt& n) = 0;
    virtual void Visit(ContinueStmt& n) = 0;
    virtual void Visit(PassStmt& n) = 0;
    virtual void Visit(ExprStatement& n) = 0;
    virtual void Visit(Block& n) = 0;
    virtual void Visit(ExtendsStmt& n) = 0;
};

// Base AST Node
struct ASTNode {
    int line = 0;
    virtual ~ASTNode() = default;
    virtual void Accept(ASTVisitor& v) = 0;
};

using ASTPtr = std::unique_ptr<ASTNode>;

// ─── Expressions ─────────────────────────────────────────
struct NumberLiteral : ASTNode {
    double value;
    bool isFloat;
    NumberLiteral(double v, bool f = false) : value(v), isFloat(f) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct StringLiteral : ASTNode {
    std::string value;
    StringLiteral(const std::string& v) : value(v) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct BoolLiteral : ASTNode {
    bool value;
    BoolLiteral(bool v) : value(v) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct NullLiteral : ASTNode {
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct Identifier : ASTNode {
    std::string name;
    Identifier(const std::string& n) : name(n) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct BinaryExpr : ASTNode {
    std::string op;
    ASTPtr left, right;
    BinaryExpr(const std::string& o, ASTPtr l, ASTPtr r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct UnaryExpr : ASTNode {
    std::string op;
    ASTPtr operand;
    UnaryExpr(const std::string& o, ASTPtr expr)
        : op(o), operand(std::move(expr)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct AssignExpr : ASTNode {
    ASTPtr target;
    std::string op; // = += -= *= /=
    ASTPtr value;
    AssignExpr(ASTPtr t, const std::string& o, ASTPtr v)
        : target(std::move(t)), op(o), value(std::move(v)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct CallExpr : ASTNode {
    ASTPtr callee;
    std::vector<ASTPtr> arguments;
    CallExpr(ASTPtr c, std::vector<ASTPtr> args)
        : callee(std::move(c)), arguments(std::move(args)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct MemberExpr : ASTNode {
    ASTPtr object;
    std::string member;
    MemberExpr(ASTPtr obj, const std::string& m)
        : object(std::move(obj)), member(m) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct IndexExpr : ASTNode {
    ASTPtr object;
    ASTPtr index;
    IndexExpr(ASTPtr obj, ASTPtr idx)
        : object(std::move(obj)), index(std::move(idx)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct ArrayLiteral : ASTNode {
    std::vector<ASTPtr> elements;
    ArrayLiteral(std::vector<ASTPtr> elems) : elements(std::move(elems)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

// ─── Statements ──────────────────────────────────────────
struct VarDecl : ASTNode {
    std::string name;
    ASTPtr initializer; // may be null
    VarDecl(const std::string& n, ASTPtr init = nullptr)
        : name(n), initializer(std::move(init)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct FuncDecl : ASTNode {
    std::string name;
    std::vector<std::string> params;
    std::vector<ASTPtr> defaultValues; // same size as params, null = no default
    ASTPtr body; // Block
    FuncDecl(const std::string& n, std::vector<std::string> p, ASTPtr b)
        : name(n), params(std::move(p)), body(std::move(b)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct ClassDecl : ASTNode {
    std::string name;
    std::string parent;
    std::vector<ASTPtr> members; // VarDecl and FuncDecl
    ClassDecl(const std::string& n) : name(n) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct ReturnStmt : ASTNode {
    ASTPtr value; // may be null
    ReturnStmt(ASTPtr v = nullptr) : value(std::move(v)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct IfStmt : ASTNode {
    ASTPtr condition;
    ASTPtr thenBranch;
    std::vector<std::pair<ASTPtr, ASTPtr>> elifBranches; // (condition, body)
    ASTPtr elseBranch; // may be null
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct WhileStmt : ASTNode {
    ASTPtr condition;
    ASTPtr body;
    WhileStmt(ASTPtr cond, ASTPtr b) : condition(std::move(cond)), body(std::move(b)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct ForStmt : ASTNode {
    std::string varName;
    ASTPtr iterable;
    ASTPtr body;
    ForStmt(const std::string& v, ASTPtr iter, ASTPtr b)
        : varName(v), iterable(std::move(iter)), body(std::move(b)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct BreakStmt : ASTNode { void Accept(ASTVisitor& v) override { v.Visit(*this); } };
struct ContinueStmt : ASTNode { void Accept(ASTVisitor& v) override { v.Visit(*this); } };
struct PassStmt : ASTNode { void Accept(ASTVisitor& v) override { v.Visit(*this); } };

struct ExprStatement : ASTNode {
    ASTPtr expression;
    ExprStatement(ASTPtr e) : expression(std::move(e)) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct Block : ASTNode {
    std::vector<ASTPtr> statements;
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct ExtendsStmt : ASTNode {
    std::string parentType;
    ExtendsStmt(const std::string& p) : parentType(p) {}
    void Accept(ASTVisitor& v) override { v.Visit(*this); }
};

struct SignalDecl : ASTNode {
    std::string name;
    SignalDecl(const std::string& n) : name(n) {}
    void Accept(ASTVisitor& v) override {}
};

struct EmitStmt : ASTNode {
    std::string signalName;
    std::vector<ASTPtr> arguments;
    void Accept(ASTVisitor& v) override {}
};

// Script is a collection of top-level statements
struct Script {
    std::string filename;
    std::string extendsType;
    std::vector<ASTPtr> statements;
};

} // namespace Nova
