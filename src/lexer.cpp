#include "lexer.hpp"
#include "token.hpp"
#include <cctype>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>

class LexException : public std::runtime_error {
public:
    LexException(const std::string& msg) : std::runtime_error(msg) {}
};

bool is_ident_head_elem(char c) noexcept {
    return isalpha(c) ||
           c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '=' || c == '<' || c == '>';
}

bool is_ident_tail_elem(char c) noexcept {
    return isdigit(c) || is_ident_head_elem(c);
}

void skip_whitespaces(std::string::const_iterator& it, const std::string::const_iterator& last) {
    while (it != last && isspace(*it)) { it++; }
}

std::shared_ptr<Token> token(std::string::const_iterator& it, const std::string::const_iterator& last) {
    skip_whitespaces(it, last);

    if (it == last) {
        throw LexException("expected character, but not found");
    }

    if (*it == '(') {
        it++;
        return std::make_shared<LParenToken>();
    } else if (*it == ')') {
        it++;
        return std::make_shared<RParenToken>();
    } else if (*it == ',') {
        it++;
        return std::make_shared<CommaToken>();
    } else if (*it == '\'') {
        it++;
        return std::make_shared<QuoteToken>();
    } else if (*it == '`') {
        it++;
        return std::make_shared<BackQuoteToken>();
    } else if (*it == '@') {
        it++;
        return std::make_shared<AtmarkToken>();
    } else if (isdigit(*it)) {
        auto rit = it;
        while (rit != last && isdigit(*rit)) { rit++; }
        if (rit != last && *rit == '.') {
            rit++;
            while (rit != last && isdigit(*rit)) { rit++; }
            auto token = std::make_shared<NumberToken>(std::stod(std::string(it, rit)));
            it = rit;
            return token;
        } else {
            auto token = std::make_shared<IntegerToken>(std::stoi(std::string(it, rit)));
            it = rit;
            return token;
        }
    } else if (*it == '"') {
        auto rit = ++it;
        while (rit != last && *rit != '"') { rit++; }
        auto token = std::make_shared<StringToken>(std::string(it, rit));
        it = rit + 1;
        return token;
    } else if (is_ident_head_elem(*it)) {
        auto rit = it;
        while (rit != last && is_ident_tail_elem(*rit)) { rit++; }
        auto token = std::make_shared<IdentifierToken>(std::string(it, rit));
        it = rit;
        return token;
    } else {
        std::ostringstream ss;
        ss << "unexpected character '" << *it << "' found";
        throw LexException(ss.str());
    }
}

std::list<std::shared_ptr<Token>> lex(const std::string& input) {
    auto it = input.begin();
    const auto last = input.end();
    std::list<std::shared_ptr<Token>> tokens = {};
    while (it != last) {
        tokens.push_back(token(it, last));
    }
    return tokens;
}
