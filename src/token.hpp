#ifndef __TOKEN_HPP__
#define __TOKEN_HPP__

#include <string>

enum class TokenKind {
    LParen,
    RParen,
    Comma,
    Quote,
    BackQuote,
    Atmark,
    Integer,
    Number,
    String,
    Identifier,
};

class Token {
public:
    virtual ~Token() {}
    virtual TokenKind kind() const = 0;
    virtual std::string debug() const = 0;
};

class LParenToken : public Token {
public:
    LParenToken();
    TokenKind kind() const override;
    std::string debug() const override;
};

class RParenToken : public Token {
public:
    RParenToken();
    TokenKind kind() const override;
    std::string debug() const override;
};

class CommaToken : public Token {
public:
    CommaToken();
    TokenKind kind() const override;
    std::string debug() const override;
};

class QuoteToken : public Token {
public:
    QuoteToken();
    TokenKind kind() const override;
    std::string debug() const override;
};

class BackQuoteToken : public Token {
public:
    BackQuoteToken();
    TokenKind kind() const override;
    std::string debug() const override;
};

class AtmarkToken : public Token {
public:
    AtmarkToken();
    TokenKind kind() const override;
    std::string debug() const override;
};

class IntegerToken : public Token {
private:
    const int __integer;

public:
    IntegerToken(int integer);
    int integer() const;
    TokenKind kind() const override;
    std::string debug() const override;
};

class NumberToken : public Token {
private:
    const double __number;

public:
    NumberToken(double number);
    double number() const;
    TokenKind kind() const override;
    std::string debug() const override;
};

class StringToken : public Token {
private:
    const std::string __string;

public:
    StringToken(const std::string& string);
    const std::string& string() const;
    TokenKind kind() const override;
    std::string debug() const override;
};

class IdentifierToken : public Token {
private:
    const std::string __identifier;

public:
    IdentifierToken(const std::string& identifier);
    const std::string& identifier() const;
    TokenKind kind() const override;
    std::string debug() const override;
};

#endif
