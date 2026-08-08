#pragma once
// ═══════════════════════════════════════════════════════════════════
//  NovaScript — Lexer (Tokenizer)
//  Converts source code into a stream of tokens
// ═══════════════════════════════════════════════════════════════════

#include <string>
#include <vector>

namespace Nova {

enum class TokenType {
    // Literals
    Integer, Float, String, True, False, Null,
    
    // Identifiers & Keywords
    Identifier,
    Var, Func, Class, Extends, Return, If, Elif, Else,
    For, While, Break, Continue, In, And, Or, Not,
    Pass, Self, Signal, Emit,
    
    // Operators
    Plus, Minus, Star, Slash, Percent, Power,         // + - * / % **
    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign, // = += -= *= /=
    Equal, NotEqual, Less, Greater, LessEqual, GreaterEqual,  // == != < > <= >=
    
    // Delimiters
    LeftParen, RightParen,     // ( )
    LeftBracket, RightBracket, // [ ]
    LeftBrace, RightBrace,     // { }
    Comma, Dot, Colon, Arrow,  // , . : ->
    
    // Structure
    Newline, Indent, Dedent,
    
    // Special
    EndOfFile, Error
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token() : type(TokenType::Error), line(0), column(0) {}
    Token(TokenType t, const std::string& v, int ln, int col)
        : type(t), value(v), line(ln), column(col) {}
    
    std::string TypeName() const;
};

class Lexer {
public:
    explicit Lexer(const std::string& source, const std::string& filename = "<script>");
    
    std::vector<Token> Tokenize();
    bool HasErrors() const { return !errors_.empty(); }
    const std::vector<std::string>& GetErrors() const { return errors_; }

private:
    char Current() const;
    char Peek(int ahead = 1) const;
    char Advance();
    bool Match(char expected);
    bool IsAtEnd() const;
    
    void SkipWhitespaceAndComments();
    Token MakeToken(TokenType type, const std::string& value);
    Token ReadString();
    Token ReadNumber();
    Token ReadIdentifierOrKeyword();
    void HandleIndentation(std::vector<Token>& tokens);
    
    void Error(const std::string& msg);
    
    std::string source_;
    std::string filename_;
    int pos_ = 0;
    int line_ = 1;
    int column_ = 1;
    bool atLineStart_ = true;
    std::vector<int> indentStack_;
    std::vector<std::string> errors_;
};

} // namespace Nova
