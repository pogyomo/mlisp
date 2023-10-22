#include "token.hpp"
#include <string>

LParenToken::LParenToken() {}

TokenKind LParenToken::kind() const {
    return TokenKind::LParen;
}

std::string LParenToken::debug() const {
    return "(";
}

RParenToken::RParenToken() {}

TokenKind RParenToken::kind() const {
    return TokenKind::RParen;
}

std::string RParenToken::debug() const {
    return ")";
}

CommaToken::CommaToken() {}

TokenKind CommaToken::kind() const {
    return TokenKind::Comma;
}

std::string CommaToken::debug() const {
    return ",";
}

QuoteToken::QuoteToken() {}

TokenKind QuoteToken::kind() const {
    return TokenKind::Quote;
}

std::string QuoteToken::debug() const {
    return "'";
}

BackQuoteToken::BackQuoteToken() {}

TokenKind BackQuoteToken::kind() const {
    return TokenKind::BackQuote;
}

std::string BackQuoteToken::debug() const {
    return "`";
}

AtmarkToken::AtmarkToken() {}

TokenKind AtmarkToken::kind() const {
    return TokenKind::Atmark;
}

std::string AtmarkToken::debug() const {
    return "@";
}

IntegerToken::IntegerToken(int integer) : __integer(integer) {}

int IntegerToken::integer() const {
    return __integer;
}

TokenKind IntegerToken::kind() const {
    return TokenKind::Integer;
}

std::string IntegerToken::debug() const {
    return std::to_string(__integer);
}

NumberToken::NumberToken(double number) : __number(number) {}

double NumberToken::number() const {
    return __number;
}

TokenKind NumberToken::kind() const {
    return TokenKind::Number;
}

std::string NumberToken::debug() const {
    return std::to_string(__number);
}

StringToken::StringToken(const std::string& string) : __string(string) {}

const std::string& StringToken::string() const {
    return __string;
}

TokenKind StringToken::kind() const {
    return TokenKind::String;
}

std::string StringToken::debug() const {
    return __string;
}

IdentifierToken::IdentifierToken(const std::string& identifier) : __identifier(identifier) {}

const std::string& IdentifierToken::identifier() const {
    return __identifier;
}

TokenKind IdentifierToken::kind() const {
    return TokenKind::Identifier;
}

std::string IdentifierToken::debug() const {
    return __identifier;
}
