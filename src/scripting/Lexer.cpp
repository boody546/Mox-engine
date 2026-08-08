#include "scripting/Lexer.h"
#include "core/Logger.h"
#include <unordered_map>

namespace Nova {

static const std::unordered_map<std::string, TokenType> keywords = {
    {"var", TokenType::Var}, {"func", TokenType::Func}, {"class", TokenType::Class},
    {"extends", TokenType::Extends}, {"return", TokenType::Return},
    {"if", TokenType::If}, {"elif", TokenType::Elif}, {"else", TokenType::Else},
    {"for", TokenType::For}, {"while", TokenType::While},
    {"break", TokenType::Break}, {"continue", TokenType::Continue},
    {"in", TokenType::In}, {"and", TokenType::And}, {"or", TokenType::Or},
    {"not", TokenType::Not}, {"true", TokenType::True}, {"false", TokenType::False},
    {"null", TokenType::Null}, {"pass", TokenType::Pass}, {"self", TokenType::Self},
    {"signal", TokenType::Signal}, {"emit", TokenType::Emit},
};

std::string Token::TypeName() const {
    switch (type) {
        case TokenType::Integer: return "Integer";
        case TokenType::Float: return "Float";
        case TokenType::String: return "String";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Newline: return "Newline";
        case TokenType::Indent: return "Indent";
        case TokenType::Dedent: return "Dedent";
        case TokenType::EndOfFile: return "EOF";
        case TokenType::Error: return "Error";
        default: return "Token(" + value + ")";
    }
}

Lexer::Lexer(const std::string& source, const std::string& filename)
    : source_(source), filename_(filename) {
    indentStack_.push_back(0);
}

char Lexer::Current() const { return IsAtEnd() ? '\0' : source_[pos_]; }
char Lexer::Peek(int ahead) const {
    int p = pos_ + ahead;
    return p < (int)source_.size() ? source_[p] : '\0';
}
char Lexer::Advance() {
    char c = Current();
    pos_++;
    column_++;
    return c;
}
bool Lexer::Match(char expected) {
    if (Current() == expected) { Advance(); return true; }
    return false;
}
bool Lexer::IsAtEnd() const { return pos_ >= (int)source_.size(); }

Token Lexer::MakeToken(TokenType type, const std::string& value) {
    return Token(type, value, line_, column_);
}

void Lexer::Error(const std::string& msg) {
    errors_.push_back(filename_ + ":" + std::to_string(line_) + ":" +
                      std::to_string(column_) + ": " + msg);
}

std::vector<Token> Lexer::Tokenize() {
    std::vector<Token> tokens;

    while (!IsAtEnd()) {
        // Handle indentation at start of line
        if (atLineStart_) {
            HandleIndentation(tokens);
            atLineStart_ = false;
            if (IsAtEnd()) break;
        }

        char c = Current();

        // Skip spaces/tabs mid-line
        if (c == ' ' || c == '\t') {
            Advance();
            continue;
        }

        // Comments
        if (c == '#') {
            while (!IsAtEnd() && Current() != '\n') Advance();
            continue;
        }

        // Newlines
        if (c == '\n') {
            Advance();
            line_++;
            column_ = 1;
            atLineStart_ = true;
            // Only add Newline if last token isn't already Newline or start
            if (!tokens.empty() && tokens.back().type != TokenType::Newline &&
                tokens.back().type != TokenType::Indent) {
                tokens.push_back(MakeToken(TokenType::Newline, "\\n"));
            }
            continue;
        }

        // Carriage return
        if (c == '\r') { Advance(); continue; }

        // Strings
        if (c == '"' || c == '\'') {
            tokens.push_back(ReadString());
            continue;
        }

        // Numbers
        if (std::isdigit(c)) {
            tokens.push_back(ReadNumber());
            continue;
        }

        // Identifiers/Keywords
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(ReadIdentifierOrKeyword());
            continue;
        }

        // Two-character operators
        Advance();
        switch (c) {
            case '+':
                if (Match('=')) tokens.push_back(MakeToken(TokenType::PlusAssign, "+="));
                else tokens.push_back(MakeToken(TokenType::Plus, "+"));
                break;
            case '-':
                if (Match('>')) tokens.push_back(MakeToken(TokenType::Arrow, "->"));
                else if (Match('=')) tokens.push_back(MakeToken(TokenType::MinusAssign, "-="));
                else tokens.push_back(MakeToken(TokenType::Minus, "-"));
                break;
            case '*':
                if (Match('*')) tokens.push_back(MakeToken(TokenType::Power, "**"));
                else if (Match('=')) tokens.push_back(MakeToken(TokenType::StarAssign, "*="));
                else tokens.push_back(MakeToken(TokenType::Star, "*"));
                break;
            case '/':
                if (Match('=')) tokens.push_back(MakeToken(TokenType::SlashAssign, "/="));
                else tokens.push_back(MakeToken(TokenType::Slash, "/"));
                break;
            case '%': tokens.push_back(MakeToken(TokenType::Percent, "%")); break;
            case '=':
                if (Match('=')) tokens.push_back(MakeToken(TokenType::Equal, "=="));
                else tokens.push_back(MakeToken(TokenType::Assign, "="));
                break;
            case '!':
                if (Match('=')) tokens.push_back(MakeToken(TokenType::NotEqual, "!="));
                else { Error("Unexpected '!'"); }
                break;
            case '<':
                if (Match('=')) tokens.push_back(MakeToken(TokenType::LessEqual, "<="));
                else tokens.push_back(MakeToken(TokenType::Less, "<"));
                break;
            case '>':
                if (Match('=')) tokens.push_back(MakeToken(TokenType::GreaterEqual, ">="));
                else tokens.push_back(MakeToken(TokenType::Greater, ">"));
                break;
            case '(': tokens.push_back(MakeToken(TokenType::LeftParen, "(")); break;
            case ')': tokens.push_back(MakeToken(TokenType::RightParen, ")")); break;
            case '[': tokens.push_back(MakeToken(TokenType::LeftBracket, "[")); break;
            case ']': tokens.push_back(MakeToken(TokenType::RightBracket, "]")); break;
            case '{': tokens.push_back(MakeToken(TokenType::LeftBrace, "{")); break;
            case '}': tokens.push_back(MakeToken(TokenType::RightBrace, "}")); break;
            case ',': tokens.push_back(MakeToken(TokenType::Comma, ",")); break;
            case '.': tokens.push_back(MakeToken(TokenType::Dot, ".")); break;
            case ':': tokens.push_back(MakeToken(TokenType::Colon, ":")); break;
            default:
                Error(std::string("Unexpected character: '") + c + "'");
                break;
        }
    }

    // Emit remaining dedents
    while (indentStack_.size() > 1) {
        indentStack_.pop_back();
        tokens.push_back(MakeToken(TokenType::Dedent, ""));
    }

    tokens.push_back(MakeToken(TokenType::EndOfFile, ""));
    return tokens;
}

void Lexer::HandleIndentation(std::vector<Token>& tokens) {
    int indent = 0;
    while (!IsAtEnd() && (Current() == ' ' || Current() == '\t')) {
        if (Current() == '\t') indent += 4;
        else indent++;
        Advance();
    }

    // Skip blank lines and comment-only lines
    if (IsAtEnd() || Current() == '\n' || Current() == '#') return;

    int currentIndent = indentStack_.back();
    if (indent > currentIndent) {
        indentStack_.push_back(indent);
        tokens.push_back(MakeToken(TokenType::Indent, ""));
    } else {
        while (indent < indentStack_.back()) {
            indentStack_.pop_back();
            tokens.push_back(MakeToken(TokenType::Dedent, ""));
        }
        if (indent != indentStack_.back()) {
            Error("Inconsistent indentation");
        }
    }
}

Token Lexer::ReadString() {
    char quote = Advance(); // consume opening quote
    std::string value;
    while (!IsAtEnd() && Current() != quote) {
        if (Current() == '\\') {
            Advance();
            switch (Current()) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default: value += Current(); break;
            }
        } else {
            value += Current();
        }
        Advance();
    }
    if (IsAtEnd()) Error("Unterminated string");
    else Advance(); // consume closing quote
    return MakeToken(TokenType::String, value);
}

Token Lexer::ReadNumber() {
    std::string value;
    bool isFloat = false;
    while (!IsAtEnd() && (std::isdigit(Current()) || Current() == '.' || Current() == '_')) {
        if (Current() == '_') { Advance(); continue; } // Allow 1_000_000
        if (Current() == '.') {
            if (isFloat) break; // Second dot, stop
            if (!std::isdigit(Peek())) break; // Dot not followed by digit
            isFloat = true;
        }
        value += Advance();
    }
    return MakeToken(isFloat ? TokenType::Float : TokenType::Integer, value);
}

Token Lexer::ReadIdentifierOrKeyword() {
    std::string value;
    while (!IsAtEnd() && (std::isalnum(Current()) || Current() == '_')) {
        value += Advance();
    }
    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return MakeToken(it->second, value);
    }
    return MakeToken(TokenType::Identifier, value);
}

} // namespace Nova
