#include "scripting/Parser.h"
#include "core/Logger.h"

namespace Nova {

static Token EOF_TOKEN(TokenType::EndOfFile, "", 0, 0);

Parser::Parser(const std::vector<Token>& tokens, const std::string& filename)
    : tokens_(tokens), filename_(filename) {}

const Token& Parser::Current() const {
    return pos_ < (int)tokens_.size() ? tokens_[pos_] : EOF_TOKEN;
}
const Token& Parser::Peek(int ahead) const {
    int p = pos_ + ahead;
    return p < (int)tokens_.size() ? tokens_[p] : EOF_TOKEN;
}
const Token& Parser::Advance() {
    const Token& t = Current();
    if (!IsAtEnd()) pos_++;
    return t;
}
bool Parser::Check(TokenType type) const { return Current().type == type; }
bool Parser::Match(TokenType type) {
    if (Check(type)) { Advance(); return true; }
    return false;
}
Token Parser::Expect(TokenType type, const std::string& msg) {
    if (Check(type)) return Advance();
    Error(msg + " (got '" + Current().value + "')");
    return Token(TokenType::Error, "", Current().line, Current().column);
}
bool Parser::IsAtEnd() const { return Current().type == TokenType::EndOfFile; }
void Parser::SkipNewlines() { while (Match(TokenType::Newline)) {} }

void Parser::Error(const std::string& msg) {
    errors_.push_back(filename_ + ":" + std::to_string(Current().line) + ": " + msg);
}

void Parser::Synchronize() {
    while (!IsAtEnd()) {
        if (Current().type == TokenType::Newline) { Advance(); return; }
        switch (Current().type) {
            case TokenType::Func: case TokenType::Class: case TokenType::Var:
            case TokenType::If: case TokenType::While: case TokenType::For:
            case TokenType::Return: return;
            default: Advance();
        }
    }
}

std::unique_ptr<Script> Parser::Parse() {
    auto script = std::make_unique<Script>();
    script->filename = filename_;
    SkipNewlines();

    // Check for extends at top
    if (Check(TokenType::Extends)) {
        Advance();
        script->extendsType = Expect(TokenType::Identifier, "Expected type name after 'extends'").value;
        SkipNewlines();
    }

    while (!IsAtEnd()) {
        SkipNewlines();
        if (IsAtEnd()) break;
        auto stmt = ParseStatement();
        if (stmt) script->statements.push_back(std::move(stmt));
    }
    return script;
}

ASTPtr Parser::ParseStatement() {
    SkipNewlines();
    if (IsAtEnd()) return nullptr;

    switch (Current().type) {
        case TokenType::Var: return ParseVarDecl();
        case TokenType::Func: return ParseFuncDecl();
        case TokenType::Class: return ParseClassDecl();
        case TokenType::If: return ParseIfStmt();
        case TokenType::While: return ParseWhileStmt();
        case TokenType::For: return ParseForStmt();
        case TokenType::Return: return ParseReturnStmt();
        case TokenType::Break: Advance(); return std::make_unique<BreakStmt>();
        case TokenType::Continue: Advance(); return std::make_unique<ContinueStmt>();
        case TokenType::Pass: Advance(); return std::make_unique<PassStmt>();
        case TokenType::Signal: {
            Advance();
            auto name = Expect(TokenType::Identifier, "Expected signal name").value;
            return std::make_unique<SignalDecl>(name);
        }
        default: {
            auto expr = ParseExpression();
            return std::make_unique<ExprStatement>(std::move(expr));
        }
    }
}

ASTPtr Parser::ParseVarDecl() {
    Expect(TokenType::Var, "Expected 'var'");
    auto name = Expect(TokenType::Identifier, "Expected variable name").value;
    ASTPtr init = nullptr;
    if (Match(TokenType::Assign)) {
        init = ParseExpression();
    }
    return std::make_unique<VarDecl>(name, std::move(init));
}

ASTPtr Parser::ParseFuncDecl() {
    Expect(TokenType::Func, "Expected 'func'");
    auto name = Expect(TokenType::Identifier, "Expected function name").value;
    Expect(TokenType::LeftParen, "Expected '(' after function name");
    auto params = ParseParameters();
    Expect(TokenType::RightParen, "Expected ')' after parameters");
    Expect(TokenType::Colon, "Expected ':' after function declaration");
    auto body = ParseBlock();
    return std::make_unique<FuncDecl>(name, std::move(params), std::move(body));
}

ASTPtr Parser::ParseClassDecl() {
    Expect(TokenType::Class, "Expected 'class'");
    auto name = Expect(TokenType::Identifier, "Expected class name").value;
    auto cls = std::make_unique<ClassDecl>(name);
    if (Match(TokenType::Extends)) {
        cls->parent = Expect(TokenType::Identifier, "Expected parent class").value;
    }
    Expect(TokenType::Colon, "Expected ':'");
    SkipNewlines();
    Expect(TokenType::Indent, "Expected indented class body");
    SkipNewlines();
    while (!Check(TokenType::Dedent) && !IsAtEnd()) {
        SkipNewlines();
        if (Check(TokenType::Dedent)) break;
        cls->members.push_back(ParseStatement());
        SkipNewlines();
    }
    Match(TokenType::Dedent);
    return cls;
}

ASTPtr Parser::ParseIfStmt() {
    Expect(TokenType::If, "Expected 'if'");
    auto ifStmt = std::make_unique<IfStmt>();
    ifStmt->condition = ParseExpression();
    Expect(TokenType::Colon, "Expected ':'");
    ifStmt->thenBranch = ParseBlock();
    SkipNewlines();

    // elif branches
    while (Check(TokenType::Elif)) {
        Advance();
        auto cond = ParseExpression();
        Expect(TokenType::Colon, "Expected ':'");
        auto body = ParseBlock();
        ifStmt->elifBranches.push_back({std::move(cond), std::move(body)});
        SkipNewlines();
    }

    // else branch
    if (Check(TokenType::Else)) {
        Advance();
        Expect(TokenType::Colon, "Expected ':'");
        ifStmt->elseBranch = ParseBlock();
    }
    return ifStmt;
}

ASTPtr Parser::ParseWhileStmt() {
    Expect(TokenType::While, "Expected 'while'");
    auto cond = ParseExpression();
    Expect(TokenType::Colon, "Expected ':'");
    auto body = ParseBlock();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

ASTPtr Parser::ParseForStmt() {
    Expect(TokenType::For, "Expected 'for'");
    auto varName = Expect(TokenType::Identifier, "Expected variable").value;
    Expect(TokenType::In, "Expected 'in'");
    auto iter = ParseExpression();
    Expect(TokenType::Colon, "Expected ':'");
    auto body = ParseBlock();
    return std::make_unique<ForStmt>(varName, std::move(iter), std::move(body));
}

ASTPtr Parser::ParseReturnStmt() {
    Expect(TokenType::Return, "Expected 'return'");
    ASTPtr val = nullptr;
    if (!Check(TokenType::Newline) && !Check(TokenType::Dedent) && !IsAtEnd()) {
        val = ParseExpression();
    }
    return std::make_unique<ReturnStmt>(std::move(val));
}

ASTPtr Parser::ParseBlock() {
    SkipNewlines();
    if (!Match(TokenType::Indent)) {
        // Single-line block
        auto block = std::make_unique<Block>();
        block->statements.push_back(ParseStatement());
        return block;
    }
    auto block = std::make_unique<Block>();
    SkipNewlines();
    while (!Check(TokenType::Dedent) && !IsAtEnd()) {
        SkipNewlines();
        if (Check(TokenType::Dedent)) break;
        auto stmt = ParseStatement();
        if (stmt) block->statements.push_back(std::move(stmt));
        SkipNewlines();
    }
    Match(TokenType::Dedent);
    return block;
}

// ─── Expression Parsing (Precedence Climbing) ───────────
ASTPtr Parser::ParseExpression() { return ParseAssignment(); }

ASTPtr Parser::ParseAssignment() {
    auto expr = ParseOr();
    if (Check(TokenType::Assign) || Check(TokenType::PlusAssign) ||
        Check(TokenType::MinusAssign) || Check(TokenType::StarAssign) ||
        Check(TokenType::SlashAssign)) {
        auto op = Advance().value;
        auto value = ParseOr();
        return std::make_unique<AssignExpr>(std::move(expr), op, std::move(value));
    }
    return expr;
}

ASTPtr Parser::ParseOr() {
    auto left = ParseAnd();
    while (Match(TokenType::Or)) {
        auto right = ParseAnd();
        left = std::make_unique<BinaryExpr>("or", std::move(left), std::move(right));
    }
    return left;
}

ASTPtr Parser::ParseAnd() {
    auto left = ParseNot();
    while (Match(TokenType::And)) {
        auto right = ParseNot();
        left = std::make_unique<BinaryExpr>("and", std::move(left), std::move(right));
    }
    return left;
}

ASTPtr Parser::ParseNot() {
    if (Match(TokenType::Not)) {
        auto expr = ParseNot();
        return std::make_unique<UnaryExpr>("not", std::move(expr));
    }
    return ParseComparison();
}

ASTPtr Parser::ParseComparison() {
    auto left = ParseAddition();
    while (Check(TokenType::Equal) || Check(TokenType::NotEqual) ||
           Check(TokenType::Less) || Check(TokenType::Greater) ||
           Check(TokenType::LessEqual) || Check(TokenType::GreaterEqual)) {
        auto op = Advance().value;
        auto right = ParseAddition();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTPtr Parser::ParseAddition() {
    auto left = ParseMultiplication();
    while (Check(TokenType::Plus) || Check(TokenType::Minus)) {
        auto op = Advance().value;
        auto right = ParseMultiplication();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTPtr Parser::ParseMultiplication() {
    auto left = ParseUnary();
    while (Check(TokenType::Star) || Check(TokenType::Slash) || Check(TokenType::Percent)) {
        auto op = Advance().value;
        auto right = ParseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

ASTPtr Parser::ParseUnary() {
    if (Check(TokenType::Minus)) {
        auto op = Advance().value;
        auto expr = ParsePower();
        return std::make_unique<UnaryExpr>(op, std::move(expr));
    }
    return ParsePower();
}

ASTPtr Parser::ParsePower() {
    auto left = ParsePostfix();
    if (Match(TokenType::Power)) {
        auto right = ParseUnary(); // Right-associative
        left = std::make_unique<BinaryExpr>("**", std::move(left), std::move(right));
    }
    return left;
}

ASTPtr Parser::ParsePostfix() {
    auto expr = ParsePrimary();
    while (true) {
        if (Match(TokenType::LeftParen)) {
            auto args = ParseArguments();
            Expect(TokenType::RightParen, "Expected ')'");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
        } else if (Match(TokenType::Dot)) {
            auto member = Expect(TokenType::Identifier, "Expected member name").value;
            expr = std::make_unique<MemberExpr>(std::move(expr), member);
        } else if (Match(TokenType::LeftBracket)) {
            auto idx = ParseExpression();
            Expect(TokenType::RightBracket, "Expected ']'");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(idx));
        } else break;
    }
    return expr;
}

ASTPtr Parser::ParsePrimary() {
    if (Check(TokenType::Integer)) {
        auto val = std::stod(Advance().value);
        return std::make_unique<NumberLiteral>(val, false);
    }
    if (Check(TokenType::Float)) {
        auto val = std::stod(Advance().value);
        return std::make_unique<NumberLiteral>(val, true);
    }
    if (Check(TokenType::String)) {
        return std::make_unique<StringLiteral>(Advance().value);
    }
    if (Check(TokenType::True)) { Advance(); return std::make_unique<BoolLiteral>(true); }
    if (Check(TokenType::False)) { Advance(); return std::make_unique<BoolLiteral>(false); }
    if (Check(TokenType::Null)) { Advance(); return std::make_unique<NullLiteral>(); }
    if (Check(TokenType::Self)) { Advance(); return std::make_unique<Identifier>("self"); }
    if (Check(TokenType::Identifier)) {
        return std::make_unique<Identifier>(Advance().value);
    }
    if (Match(TokenType::LeftParen)) {
        auto expr = ParseExpression();
        Expect(TokenType::RightParen, "Expected ')'");
        return expr;
    }
    if (Check(TokenType::LeftBracket)) {
        return ParseArrayLiteral();
    }

    Error("Unexpected token: '" + Current().value + "'");
    Advance();
    return std::make_unique<NullLiteral>();
}

ASTPtr Parser::ParseArrayLiteral() {
    Expect(TokenType::LeftBracket, "Expected '['");
    std::vector<ASTPtr> elems;
    if (!Check(TokenType::RightBracket)) {
        elems.push_back(ParseExpression());
        while (Match(TokenType::Comma)) {
            if (Check(TokenType::RightBracket)) break;
            elems.push_back(ParseExpression());
        }
    }
    Expect(TokenType::RightBracket, "Expected ']'");
    return std::make_unique<ArrayLiteral>(std::move(elems));
}

std::vector<ASTPtr> Parser::ParseArguments() {
    std::vector<ASTPtr> args;
    if (!Check(TokenType::RightParen)) {
        args.push_back(ParseExpression());
        while (Match(TokenType::Comma)) {
            args.push_back(ParseExpression());
        }
    }
    return args;
}

std::vector<std::string> Parser::ParseParameters() {
    std::vector<std::string> params;
    if (!Check(TokenType::RightParen)) {
        params.push_back(Expect(TokenType::Identifier, "Expected parameter name").value);
        while (Match(TokenType::Comma)) {
            params.push_back(Expect(TokenType::Identifier, "Expected parameter name").value);
        }
    }
    return params;
}

} // namespace Nova
