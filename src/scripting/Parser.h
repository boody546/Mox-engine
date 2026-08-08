#pragma once
// ═══════════════════════════════════════════════════════════════════
//  NovaScript — Parser
//  Converts token stream into an Abstract Syntax Tree
// ═══════════════════════════════════════════════════════════════════

#include "scripting/Lexer.h"
#include "scripting/AST.h"
#include <vector>
#include <string>
#include <memory>

namespace Nova {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens, const std::string& filename = "<script>");

    std::unique_ptr<Script> Parse();
    bool HasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& GetErrors() const { return errors_; }

private:
    // Token access
    const Token& Current() const;
    const Token& Peek(int ahead = 1) const;
    const Token& Advance();
    bool Check(TokenType type) const;
    bool Match(TokenType type);
    Token Expect(TokenType type, const std::string& msg);
    bool IsAtEnd() const;
    void SkipNewlines();

    // Statements
    ASTPtr ParseStatement();
    ASTPtr ParseVarDecl();
    ASTPtr ParseFuncDecl();
    ASTPtr ParseClassDecl();
    ASTPtr ParseIfStmt();
    ASTPtr ParseWhileStmt();
    ASTPtr ParseForStmt();
    ASTPtr ParseReturnStmt();
    ASTPtr ParseBlock();

    // Expressions (precedence climbing)
    ASTPtr ParseExpression();
    ASTPtr ParseAssignment();
    ASTPtr ParseOr();
    ASTPtr ParseAnd();
    ASTPtr ParseNot();
    ASTPtr ParseComparison();
    ASTPtr ParseAddition();
    ASTPtr ParseMultiplication();
    ASTPtr ParseUnary();
    ASTPtr ParsePower();
    ASTPtr ParsePostfix();
    ASTPtr ParsePrimary();
    ASTPtr ParseArrayLiteral();

    // Helpers
    std::vector<ASTPtr> ParseArguments();
    std::vector<std::string> ParseParameters();

    void Error(const std::string& msg);
    void Synchronize();

    std::vector<Token> tokens_;
    std::string filename_;
    int pos_ = 0;
    std::vector<std::string> errors_;
};

} // namespace Nova
