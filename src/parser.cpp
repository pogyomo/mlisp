#include "object.hpp"
#include "parser.hpp"
#include "token.hpp"
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>

class ParseException : public std::runtime_error {
public:
    ParseException(const std::string& msg) : std::runtime_error(msg) {}
};

std::shared_ptr<Object> parse_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<Object> parse_list_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<IntegerObject> parse_integer_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<NumberObject> parse_number_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<StringObject> parse_string_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<SymbolObject> parse_symbol_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<QuotedObject> parse_quoted_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<BackQuotedObject> parse_back_quoted_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<CommaObject> parse_comma_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<CommaAtmarkObject> parse_comma_atmark_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
);

std::shared_ptr<Object> parse_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    }

    switch ((*it)->kind()) {
        case TokenKind::LParen:
            return parse_list_expression(it, last);
        case TokenKind::Integer:
            return parse_integer_expression(it, last);
        case TokenKind::Number:
            return parse_number_expression(it, last);
        case TokenKind::String:
            return parse_string_expression(it, last);
        case TokenKind::Identifier:
            return parse_symbol_expression(it, last);
        case TokenKind::Quote:
            return parse_quoted_expression(it, last);
        case TokenKind::BackQuote:
            return parse_back_quoted_expression(it, last);
        case TokenKind::Comma:
            if ((*++it)->kind() == TokenKind::Atmark) {
                it--;
                return parse_comma_atmark_expression(it, last);
            } else {
                it--;
                return parse_comma_expression(it, last);
            }
        default:
            std::ostringstream ss;
            ss << "expected (, integer, number, string, identifier, ', `, @ or ',', but got ";
            ss << (*it)-> debug();
            throw ParseException(ss.str());
    }
}

std::shared_ptr<Object> parse_list_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::LParen) {
        throw ParseException("expected (, but got " + (*it)->debug());
    } else {
        it++;
    }

    std::shared_ptr<Object> elem;
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() == TokenKind::RParen) {
        it++;
        return std::make_shared<NILObject>();
    } else {
        elem = parse_expression(it, last);
    }

    std::shared_ptr<ListObject> list = std::make_shared<ListObject>(elem);
    while (true) {
        if (it == last) {
            throw ParseException("expected token, but not found");
        } else if ((*it)->kind() == TokenKind::RParen) {
            it++;
            break;
        } else {
            list->append(parse_expression(it, last));
        }
    }
    return list;
}

std::shared_ptr<IntegerObject> parse_integer_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Integer) {
        throw ParseException("expected integer, but got " + (*it)->debug());
    }

    auto integer = std::static_pointer_cast<IntegerToken>(*it)->integer();
    it++;
    return std::make_shared<IntegerObject>(integer);
}

std::shared_ptr<NumberObject> parse_number_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Number) {
        throw ParseException("expected number, but got " + (*it)->debug());
    }

    auto number = std::static_pointer_cast<NumberToken>(*it)->number();
    it++;
    return std::make_shared<NumberObject>(number);
}

std::shared_ptr<StringObject> parse_string_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::String) {
        throw ParseException("expected string, but got " + (*it)->debug());
    }

    auto string = std::static_pointer_cast<StringToken>(*it)->string();
    it++;
    return std::make_shared<StringObject>(string);
}

std::shared_ptr<SymbolObject> parse_symbol_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Identifier) {
        throw ParseException("expected symbol, but got " + (*it)->debug());
    }

    auto symbol = std::static_pointer_cast<IdentifierToken>(*it)->identifier();
    it++;
    return std::make_shared<SymbolObject>(symbol);
}

std::shared_ptr<QuotedObject> parse_quoted_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Quote) {
        throw ParseException("expected ', but got " + (*it)->debug());
    } else {
        it++;
    }

    return std::make_shared<QuotedObject>(parse_expression(it, last));
}

std::shared_ptr<BackQuotedObject> parse_back_quoted_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::BackQuote) {
        throw ParseException("expected `, but got " + (*it)->debug());
    } else {
        it++;
    }

    return std::make_shared<BackQuotedObject>(parse_expression(it, last));
}

std::shared_ptr<CommaObject> parse_comma_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Comma) {
        throw ParseException("expected ',', but got " + (*it)->debug());
    } else {
        it++;
    }

    return std::make_shared<CommaObject>(parse_expression(it, last));
}

std::shared_ptr<CommaAtmarkObject> parse_comma_atmark_expression(
    std::list<std::shared_ptr<Token>>::const_iterator& it, 
    const std::list<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Comma) {
        throw ParseException("expected ',', but got " + (*it)->debug());
    } else {
        it++;
    }

    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Atmark) {
        throw ParseException("expected @, but got " + (*it)->debug());
    } else {
        it++;
    }

    return std::make_shared<CommaAtmarkObject>(parse_expression(it, last));
}

std::list<std::shared_ptr<Object>> parse(const std::list<std::shared_ptr<Token>>& tokens) {
    auto it = tokens.begin();
    const auto last = tokens.end();
    std::list<std::shared_ptr<Object>> exprs;
    while (it != last) {
        exprs.push_back(parse_expression(it, last));
    }
    return exprs;
}
