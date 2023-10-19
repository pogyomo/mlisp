#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <istream>
#include <iterator>
#include <sstream>
#include <list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

enum class TokenKind {
    LParen,
    RParen,
    Quote,
    Ident,
    Integer,
    Number,
    String,
};

class Token {
public:
    virtual ~Token() {}
    virtual TokenKind kind() const = 0;
    virtual std::string debug() const = 0;
};

class LParenToken : public Token {
public:
    LParenToken() {}

    TokenKind kind() const override {
        return TokenKind::LParen;
    }

    std::string debug() const override {
        return "(";
    }
};

class RParenToken : public Token {
public:
    RParenToken() {}

    TokenKind kind() const override {
        return TokenKind::RParen;
    }

    std::string debug() const override {
        return ")";
    }
};

class QuoteToken : public Token {
public:
    QuoteToken() {}

    TokenKind kind() const override {
        return TokenKind::Quote;
    }

    std::string debug() const override {
        return "'";
    }
};

class IdentToken : public Token {
private:
    std::string ident;

public:
    IdentToken(const std::string& ident) {
        this->ident = ident;
    }

    std::string& get_ident() {
        return ident;
    }

    TokenKind kind() const override {
        return TokenKind::Ident;
    }

    std::string debug() const override {
        return ident;
    }
};

class IntegerToken : public Token {
private:
    int integer;

public:
    IntegerToken(int integer) {
        this->integer = integer;
    }

    int get_integer() {
        return integer;
    }

    TokenKind kind() const override {
        return TokenKind::Integer;
    }

    std::string debug() const override {
        return std::to_string(integer);
    }
};

class NumberToken : public Token {
private:
    double number;

public:
    NumberToken(double number) {
        this->number = number;
    }

    double get_number() {
        return number;
    }

    TokenKind kind() const override {
        return TokenKind::Number;
    }

    std::string debug() const override {
        return std::to_string(number);
    }
};

class StringToken : public Token {
private:
    std::string string;

public:
    StringToken(const std::string& string) {
        this->string = string;
    }

    std::string& get_string() {
        return string;
    }

    TokenKind kind() const override {
        return TokenKind::String;
    }

    std::string debug() const override {
        return "\"" + string + "\"";
    }
};

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

std::shared_ptr<Token> token(std::string::const_iterator& it, const std::string::const_iterator& last) {
    assert(it != last);
    if (*it == '(') {
        it++;
        return std::make_shared<LParenToken>();
    } else if (*it == ')') {
        it++;
        return std::make_shared<RParenToken>();
    } else if (*it == '\'') {
        it++;
        return std::make_shared<QuoteToken>();
    } else if (is_ident_head_elem(*it)) {
        auto rit = it;
        while (rit != last && is_ident_tail_elem(*rit)) { rit++; }
        auto token = std::make_shared<IdentToken>(std::string(it, rit));
        it = rit;
        return token;
    } else if (*it == '"') {
        auto rit = ++it;
        while (rit != last && *rit != '"') { rit++; }
        auto token = std::make_shared<StringToken>(std::string(it, rit));
        it = rit + 1;
        return token;
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
    } else {
        std::ostringstream ss;
        ss << "unexpected character '" << *it << "' found";
        throw LexException(ss.str());
    }
}

bool skip_whitespaces(std::string::const_iterator& it, const std::string::const_iterator& last) {
    if (it != last && isspace(*it)) {
        while (it != last && isspace(*it)) { it++; }
        return true;
    } else {
        return false;
    }
}

std::vector<std::shared_ptr<Token>> lex(const std::string& input) {
    std::vector<std::shared_ptr<Token>> tokens;
    auto it = input.begin();
    const auto last = input.end();
    while (true) {
        skip_whitespaces(it, last);
        if (it != last) {
            tokens.push_back(token(it, last));
        } else {
            break;
        }
    }
    return tokens;
}

enum class ObjectKind {
    // Objects which user can create.
    List,
    T,
    NIL,
    Integer,
    Number,
    String,
    Symbol,
    Function,
    PartiallyAppliedFunction,
    Macro,

    // Objects which internally used and user can't create these object.
    FuncPtr,
    PartiallyAppliedFuncPtr,
};

class Object {
public:
    virtual ~Object() {}
    virtual ObjectKind kind() const = 0;
    virtual bool is_atom() const = 0;
    virtual std::string debug() const = 0;
};

class EnvException : public std::runtime_error {
public:
    EnvException(const std::string& msg) : std::runtime_error(msg) {}
};

// This use `Object` and `FuncPtr` use this, so this must be placed between `Object` and `FuncPtr`.
class Env {
private:
    std::map<std::string, std::shared_ptr<Object>> symtable;
    std::shared_ptr<Env> outer;

public:
    Env() {
        this->outer = nullptr;
    }

    Env(std::shared_ptr<Env> outer) {
        this->outer = outer;
    }

    std::shared_ptr<Object> get_obj(const std::string& sym) {
        try {
            return symtable.at(sym);
        } catch (std::out_of_range& _) {
            if (outer != nullptr) {
                return outer->get_obj(sym);
            } else {
                throw EnvException("no such symbol exist: " + sym);
            }
        }
    }

    void set_obj(const std::string& sym, const std::shared_ptr<Object> obj) {
        symtable[sym] = obj;
    }
};

// A list which has one value and maybe have rest.
class List : public Object, public std::enable_shared_from_this<List> {
private:
    std::shared_ptr<Object> value;
    std::shared_ptr<List> next;

public:
    List(std::shared_ptr<Object> value) {
        this->value = value;
        this->next = nullptr;
    }

    List(std::shared_ptr<Object> value, std::shared_ptr<List> next) {
        this->value = value;
        this->next = next;
    }

    void append(std::shared_ptr<List> list) {
        auto it = shared_from_this();
        while (it->next != nullptr) {
            it = it->next;
        }
        it->next = list;
    }

    std::list<std::shared_ptr<Object>> to_list() {
        std::list<std::shared_ptr<Object>> list;
        auto it = shared_from_this();
        while (it != nullptr) {
            list.push_back(it->value);
            it = it->next;
        }
        return list;
    }

    std::shared_ptr<Object> get_value() {
        return value;
    }

    std::shared_ptr<List> get_next() {
        return next;
    }

    ObjectKind kind() const override {
        return ObjectKind::List;
    }

    bool is_atom() const override {
        return false;
    }

    std::string debug() const override {
        std::string s = "(";
        auto list = shared_from_this();
        while (list != nullptr) {
            s += list->value->debug() + " ";
            list = list->next;
        }
        return s.substr(0, s.size() - 1) + ")";
    }
};

class T : public Object {
public:
    ObjectKind kind() const override {
        return ObjectKind::T;
    }

    bool is_atom() const override {
        return true;
    }

    std::string debug() const override {
        return "T";
    }
};

class NIL : public Object {
public:
    ObjectKind kind() const override {
        return ObjectKind::NIL;
    }

    bool is_atom() const override {
        return true;
    }

    std::string debug() const override {
        return "NIL";
    }
};

class Integer : public Object {
private:
    int integer;

public:
    Integer(int integer) {
        this->integer = integer;
    }

    int get_integer() {
        return integer;
    }

    ObjectKind kind() const override {
        return ObjectKind::Integer;
    }

    bool is_atom() const override {
        return true;
    }

    std::string debug() const override {
        return std::to_string(integer);
    }
};

class Number : public Object {
private:
    double number;

public:
    Number(double number) {
        this->number = number;
    }

    double get_number() {
        return number;
    }

    ObjectKind kind() const override {
        return ObjectKind::Number;
    }

    bool is_atom() const override {
        return true;
    }

    std::string debug() const override {
        return std::to_string(number);
    }
};

class String : public Object {
private:
    std::string string;

public:
    String(std::string string) {
        this->string = string;
    }

    std::string& get_string() {
        return string;
    }

    ObjectKind kind() const override {
        return ObjectKind::String;
    }

    bool is_atom() const override {
        return true;
    }

    std::string debug() const override {
        return "\"" + string + "\"";
    }
};

class Symbol : public Object {
private:
    std::string symbol;

public:
    Symbol(std::string symbol) {
        this->symbol = symbol;
    }

    std::string& get_symbol() {
        return symbol;
    }

    ObjectKind kind() const override {
        return ObjectKind::Symbol;
    }

    bool is_atom() const override {
        return true;
    }

    std::string debug() const override {
        return symbol;
    }
};

class Function : public Object {
private:
    std::list<std::shared_ptr<Symbol>> params;
    std::list<std::shared_ptr<Object>> body;

public:
    Function(std::list<std::shared_ptr<Symbol>> params, std::list<std::shared_ptr<Object>> body) {
        this->params = params;
        this->body = body;
    }

    std::list<std::shared_ptr<Symbol>>& get_params() {
        return params;
    }

    std::list<std::shared_ptr<Object>>& get_body() {
        return body;
    }

    ObjectKind kind() const override {
        return ObjectKind::Function;
    }

    bool is_atom() const override {
        return false;
    }

    std::string debug() const override {
        std::ostringstream ss;
        ss << "FUNCTION (";
        for (auto it = params.begin(); it != params.end(); it++) {
            if (++it == params.end()) {
                it--;
                ss << (*it)->debug();
            } else {
                it--;
                ss << (*it)->debug() << " ";
            }
        }
        ss << ")";
        for (const auto& body : body) {
            ss << " " << body->debug();
        }
        std::string s = ss.str();
        return s.substr(0, s.size());
    }
};

class PartiallyAppliedFunction : public Object {
private:
    std::shared_ptr<Function> func;
    std::shared_ptr<List> args;

public:
    PartiallyAppliedFunction(std::shared_ptr<Function> func) {
        this->func = func;
        this->args = {};
    }

    PartiallyAppliedFunction(std::shared_ptr<Function> func, std::shared_ptr<List> args) {
        this->func = func;
        this->args = args;
    }

    std::shared_ptr<Function> get_func() {
        return func;
    }

    std::shared_ptr<List>& get_args() {
        return args;
    }

    ObjectKind kind() const override {
        return ObjectKind::PartiallyAppliedFunction;
    }

    bool is_atom() const override {
        return false;
    }

    std::string debug() const override {
        std::string s = func->debug();
        auto arg_it = args;
        while(arg_it != nullptr) {
            s += " " + arg_it->get_value()->debug();
            arg_it = arg_it->get_next();
        }
        return s;
    }
};

class FuncPtr : public Object {
private:
    std::function<std::shared_ptr<Object>(const std::shared_ptr<List>, Env&)> func;

public:
    FuncPtr(std::function<std::shared_ptr<Object>(const std::shared_ptr<List>, Env&)> func) {
        this->func = func;
    }

    std::function<std::shared_ptr<Object>(const std::shared_ptr<List>, Env&)>& get_func() {
        return func;
    };

    ObjectKind kind() const override {
        return ObjectKind::FuncPtr;
    }

    bool is_atom() const override {
        return false;
    }

    std::string debug() const override {
        return "buildin function";
    }
};

class PartiallyAppliedFuncPtr : public Object {
private:
    std::shared_ptr<FuncPtr> func;
    std::shared_ptr<List> args;

public:
    PartiallyAppliedFuncPtr(std::shared_ptr<FuncPtr> func) {
        this->func = func;
        this->args = {};
    }

    PartiallyAppliedFuncPtr(std::shared_ptr<FuncPtr> func, std::shared_ptr<List> args) {
        this->func = func;
        this->args = args;
    }

    std::shared_ptr<FuncPtr> get_func() {
        return func;
    }

    std::shared_ptr<List> get_args() {
        return args;
    }

    ObjectKind kind() const override {
        return ObjectKind::PartiallyAppliedFuncPtr;
    }

    bool is_atom() const override {
        return false;
    }

    std::string debug() const override {
        return "partially applied buildin function";
    }
};

class Macro : public Object {
private:
    std::list<std::shared_ptr<Symbol>> params;
    std::list<std::shared_ptr<Object>> body;

public:
    Macro(std::list<std::shared_ptr<Symbol>> params, std::list<std::shared_ptr<Object>> body) {
        this->params = params;
        this->body = body;
    }

    std::list<std::shared_ptr<Symbol>>& get_params() {
        return params;
    }

    std::list<std::shared_ptr<Object>>& get_body() {
        return body;
    }

    ObjectKind kind() const override {
        return ObjectKind::Macro;
    }

    bool is_atom() const override {
        return false;
    }

    std::string debug() const override {
        std::ostringstream ss;
        ss << "MACRO (";
        for (auto it = params.begin(); it != params.end(); it++) {
            if (++it == params.end()) {
                it--;
                ss << (*it)->debug();
            } else {
                it--;
                ss << (*it)->debug() << " ";
            }
        }
        ss << ")";
        for (const auto& body : body) {
            ss << " " << body->debug();
        }
        std::string s = ss.str();
        return s.substr(0, s.size());
    }
};

static std::shared_ptr<T> GLOBAL_T = std::make_shared<T>();
static std::shared_ptr<NIL> GLOBAL_NIL = std::make_shared<NIL>();

class ParseException : public std::runtime_error {
public:
    ParseException(const std::string& msg) : std::runtime_error(msg) {}
};

std::vector<std::shared_ptr<Object>> parse(const std::vector<std::shared_ptr<Token>>& tokens);
std::shared_ptr<Object> parse_object(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<Integer> parse_int(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<Number> parse_num(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<String> parse_str(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<Symbol> parse_sym(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<Object> parse_list(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
);
std::shared_ptr<List> parse_quote(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
);

std::vector<std::shared_ptr<Object>> parse(const std::vector<std::shared_ptr<Token>>& tokens) {
    std::vector<std::shared_ptr<Object>> atoms = {};
    auto it = tokens.begin();
    const auto last = tokens.end();
    while (it != tokens.end()) {
        atoms.push_back(parse_object(it, last));
    }
    return atoms;
}

std::shared_ptr<Object> parse_object(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    }

    switch ((*it)->kind()) {
        case TokenKind::Integer:
            return parse_int(it, last);
        case TokenKind::Number:
            return parse_num(it, last);
        case TokenKind::String:
            return parse_str(it, last);
        case TokenKind::LParen:
            return parse_list(it, last);
        case TokenKind::Ident:
            return parse_sym(it, last);
        case TokenKind::Quote:
            return parse_quote(it, last);
        default:
            std::ostringstream ss;
            ss << "unexpected token " << (*it)->debug();
            ss << " found: expect integer, number ( or identifier";
            throw ParseException(ss.str());
    }
}

std::shared_ptr<Integer> parse_int(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Integer) {
        std::ostringstream ss;
        ss << "unexpected token " << (*it)->debug() << " found: expected integer";
        throw ParseException(ss.str());
    } else {
        const std::shared_ptr<IntegerToken> integer = std::dynamic_pointer_cast<IntegerToken>(*it);
        it++;
        return std::make_shared<Integer>(integer->get_integer());
    }
}

std::shared_ptr<Number> parse_num(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Number) {
        std::ostringstream ss;
        ss << "unexpected token " << (*it)->debug() << " found: expected number";
        throw ParseException(ss.str());
    } else {
        const std::shared_ptr<NumberToken> number = std::dynamic_pointer_cast<NumberToken>(*it);
        it++;
        return std::make_shared<Number>(number->get_number());
    }
}

std::shared_ptr<String> parse_str(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::String) {
        std::ostringstream ss;
        ss << "unexpected token " << (*it)->debug() << " found: expected string";
        throw ParseException(ss.str());
    } else {
        const std::shared_ptr<StringToken> string = std::dynamic_pointer_cast<StringToken>(*it);
        it++;
        return std::make_shared<String>(string->get_string());
    }
}

std::shared_ptr<Symbol> parse_sym(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Ident) {
        std::ostringstream ss;
        ss << "unexpected token " << (*it)->debug() << " found: expected identifier";
        throw ParseException(ss.str());
    } else {
        const std::shared_ptr<IdentToken> ident = std::dynamic_pointer_cast<IdentToken>(*it);
        it++;
        return std::make_shared<Symbol>(ident->get_ident());
    }
}

std::shared_ptr<Object> parse_list(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::LParen) {
        std::ostringstream ss;
        ss << "unexpected token " << (*it)->debug() << " found: expected (";
        throw ParseException(ss.str());
    } else {
        it++;
    }

    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() == TokenKind::RParen) {
        it++;
        return GLOBAL_NIL;
    } else {
        std::shared_ptr<List> list = std::make_shared<List>(parse_object(it, last));
        while (true) {
            if (it == last) {
                throw ParseException("expected token, but not found");
            } else if ((*it)->kind() == TokenKind::RParen) {
                it++;
                break;
            } else {
                list->append(std::make_shared<List>(parse_object(it, last)));
            }
        }
        return list;
    }
}

std::shared_ptr<List> parse_quote(
    std::vector<std::shared_ptr<Token>>::const_iterator& it,
    const std::vector<std::shared_ptr<Token>>::const_iterator& last
) {
    if (it == last) {
        throw ParseException("expected token, but not found");
    } else if ((*it)->kind() != TokenKind::Quote) {
        std::ostringstream ss;
        ss << "unexpected token " << (*it)->debug() << " found: expected '";
        throw ParseException(ss.str());
    } else {
        it++;
        const std::shared_ptr<Object> rest = parse_object(it, last);
        return std::make_shared<List>(std::make_shared<Symbol>("quote"), std::make_shared<List>(rest));
    }
}

class EvalException : public std::runtime_error {
public:
    EvalException(const std::string& msg) : std::runtime_error(msg) {}
};

#define TAKE_JUST_ONE_ARG(name, args, a1) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() != nullptr) { \
            throw EvalException("too many arguments for " + std::string(name)); \
        } \
    } while (0)

#define TAKE_JUST_TWO_ARG(name, args, a1, a2) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a2 = args->get_next()->get_value(); \
        if (args->get_next()->get_next() != nullptr) { \
            throw EvalException("too many arguments for " + std::string(name)); \
        } \
    } while (0)

#define TAKE_JUST_THREE_ARG(name, args, a1, a2, a3) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a2 = args->get_next()->get_value(); \
        if (args->get_next()->get_next() == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a3 = args->get_next()->get_next()->get_value(); \
        if (args->get_next()->get_next()->get_next() != nullptr) { \
            throw EvalException("too many arguments for " + std::string(name)); \
        } \
    } while (0)

#define EVAL_JUST_ONE_ARG(name, args, env, a1) \
    do { \
        TAKE_JUST_ONE_ARG(name, args, a1); \
        a1 = eval(a1, env); \
    } while (0)

#define EVAL_JUST_TWO_ARG(name, args, env, a1, a2) \
    do { \
        TAKE_JUST_TWO_ARG(name, args, a1, a2); \
        a1 = eval(a1, env); \
        a2 = eval(a2, env); \
    } while (0)

#define EVAL_JUST_THREE_ARG(name, args, env, a1, a2, a3) \
    do { \
        TAKE_JUST_THREE_ARG(name, args, a1, a2, a3); \
        a1 = eval(a1, env); \
        a2 = eval(a2, env); \
        a3 = eval(a3, env); \
    } while (0) \

#define TAKE_ONE_ARG(name, args, a1) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a1 = args->get_value(); \
    } while (0)

#define TAKE_TWO_ARG(name, args, a1, a2) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a2 = args->get_next()->get_value(); \
    } while (0)

#define TAKE_THREE_ARG(name, args, a1, a2, a3) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a2 = args->get_next()->get_value(); \
        if (args->get_next()->get_next() == nullptr) { \
            throw EvalException("too few arguments for " + std::string(name)); \
        } \
        a3 = args->get_next()->get_next()->get_value(); \
    } while (0)

#define EVAL_ONE_ARG(name, args, env, a1) \
    do { \
        TAKE_ONE_ARG(name, args, a1); \
        a1 = eval(a1, env); \
    } while (0)

#define EVAL_TWO_ARG(name, args, env, a1, a2) \
    do { \
        TAKE_TWO_ARG(name, args, a1, a2); \
        a1 = eval(a1, env); \
        a2 = eval(a2, env); \
    } while (0)

#define EVAL_THREE_ARG(name, args, env, a1, a2, a3) \
    do { \
        TAKE_THREE_ARG(name, args, a1, a2, a3); \
        a1 = eval(a1, env); \
        a2 = eval(a2, env); \
        a3 = eval(a3, env); \
    } while (0) \

std::shared_ptr<Object> eval(const std::shared_ptr<Object>& object, Env& env);
std::shared_ptr<Object> eval_list(const std::shared_ptr<List>& list, Env& env);
std::shared_ptr<Object> eval_symbol(const std::shared_ptr<Symbol>& symbol, Env& env);
std::list<std::shared_ptr<Object>> expand_macro(
    const std::shared_ptr<Macro> macro,
    const std::shared_ptr<List> args,
    Env& env
);
std::shared_ptr<Object> apply_macro(
    const std::shared_ptr<Macro> macro,
    const std::shared_ptr<List> args,
    Env& env
);
std::shared_ptr<Object> apply_part_func_ptr(
    const std::shared_ptr<PartiallyAppliedFuncPtr> func,
    const std::shared_ptr<List> args,
    Env& env
);
std::shared_ptr<Object> apply_func_ptr(
    const std::shared_ptr<FuncPtr> func,
    const std::shared_ptr<List> args,
    Env& env
);
std::shared_ptr<Object> apply_part_func(
    const std::shared_ptr<PartiallyAppliedFunction> func,
    const std::shared_ptr<List> args,
    Env& env
);
std::shared_ptr<Object> apply_func(
    const std::shared_ptr<Function> func,
    const std::shared_ptr<List> args,
    Env& env
);
std::shared_ptr<Object> fn_quote(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_list(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_car(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_cdr(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_cons(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_atom(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_if(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_eq_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_ne_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_lt_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_gt_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_le_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_ge_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_add_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_sub_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_mul_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_div_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_eq_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_ne_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_lt_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_gt_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_le_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_ge_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_equal_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_write(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_write_line(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_print(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_prin1(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_princ(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_read_str(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_read_int(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_read_num(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_lambda(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_macro(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_set(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_setq(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_int_to_string(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_num_to_string(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_debug(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_type_of(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_concat(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_defun(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_defmacro(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_macroexpand(const std::shared_ptr<List> args, Env& env);

std::shared_ptr<Object> eval(const std::shared_ptr<Object>& object, Env& env) {
    switch (object->kind()) {
        case ObjectKind::T:
        case ObjectKind::NIL:
        case ObjectKind::Integer:
        case ObjectKind::Number:
        case ObjectKind::String:
            return object;
        case ObjectKind::List:
            return eval_list(std::dynamic_pointer_cast<List>(object), env);
        case ObjectKind::Symbol:
            return eval_symbol(std::dynamic_pointer_cast<Symbol>(object), env);
        default:
            // unreachable
            exit(1);
    }
}

std::shared_ptr<Object> eval_list(const std::shared_ptr<List>& list, Env& env) {
    auto head = list->get_value();
    if (head->kind() == ObjectKind::Symbol) {
        auto name = std::dynamic_pointer_cast<Symbol>(head)->get_symbol();
        auto obj = env.get_obj(name);
        if (obj->kind() == ObjectKind::FuncPtr) {
            auto func = std::dynamic_pointer_cast<FuncPtr>(obj);
            return apply_func_ptr(func, list->get_next(), env);
        } else if (obj->kind() == ObjectKind::Function) {
            return apply_func(std::dynamic_pointer_cast<Function>(obj), list->get_next(), env);
        } else if (obj->kind() == ObjectKind::PartiallyAppliedFuncPtr) {
            auto func = std::dynamic_pointer_cast<PartiallyAppliedFuncPtr>(obj);
            return apply_part_func_ptr(func, list->get_next(), env);
        } else if (obj->kind() == ObjectKind::PartiallyAppliedFunction) {
            auto func = std::dynamic_pointer_cast<PartiallyAppliedFunction>(obj);
            return apply_part_func(func, list->get_next(), env);
        } else if (obj->kind() == ObjectKind::Macro) {
            auto macro = std::dynamic_pointer_cast<Macro>(obj);
            return apply_macro(macro, list->get_next(), env);
        } else {
            throw EvalException("first symbol must be callable");
        }
    } else {
        auto obj = eval(head, env);
        if (obj->kind() == ObjectKind::Function) {
            auto func = std::dynamic_pointer_cast<Function>(obj);
            return apply_func(func, list->get_next(), env);
        } else if (obj->kind() == ObjectKind::PartiallyAppliedFunction) {
            auto func = std::dynamic_pointer_cast<PartiallyAppliedFunction>(obj);
            return apply_part_func(func, list->get_next(), env);
        } else if (obj->kind() == ObjectKind::PartiallyAppliedFuncPtr) {
            auto func = std::dynamic_pointer_cast<PartiallyAppliedFuncPtr>(obj);
            return apply_part_func_ptr(func, list->get_next(), env);
        } else if (obj->kind() == ObjectKind::Macro) {
            auto macro = std::dynamic_pointer_cast<Macro>(obj);
            return apply_macro(macro, list->get_next(), env);
        } else {
            throw EvalException("first object of list must be function or symbol");
        }
    }
}

std::list<std::shared_ptr<Object>> expand_macro(
    const std::shared_ptr<Macro> macro,
    const std::shared_ptr<List> args,
    Env& env
) {
    std::list<std::shared_ptr<Object>> arg_list;
    auto head = args;
    while (head != nullptr) {
        arg_list.push_back(head->get_value());
        head = head->get_next();
    }

    Env temp_env(env);
    if (arg_list.size() != macro->get_params().size()) {
        std::ostringstream ss;
        ss << "different number of argument to macro: expect " << macro->get_params().size();
        ss << ", but got " << arg_list.size();
        throw EvalException(ss.str());
    } else {
        auto syms = macro->get_params().begin();
        auto args = arg_list.begin();
        while (syms != macro->get_params().end()) {
            temp_env.set_obj((*syms)->get_symbol(), *args);
            syms++; args++;
        }
    }

    std::list<std::shared_ptr<Object>> list;
    for (auto& body : macro->get_body()) {
        list.push_back(eval(body, temp_env));
    }
    return list;
}

std::shared_ptr<Object> apply_macro(
    const std::shared_ptr<Macro> macro,
    const std::shared_ptr<List> args,
    Env& env
) {
    auto expanded_objs = expand_macro(macro, args, env);
    std::shared_ptr<Object> result = GLOBAL_NIL;
    for (auto& expanded_obj : expanded_objs) {
        result = eval(expanded_obj, env);
    }
    return result;
}

std::shared_ptr<Object> apply_part_func_ptr(
    const std::shared_ptr<PartiallyAppliedFuncPtr> func,
    const std::shared_ptr<List> args,
    Env& env
) {
    auto new_args = func->get_args();
    new_args->append(args);
    return apply_func_ptr(func->get_func(), new_args, env);
}

std::shared_ptr<Object> apply_func_ptr(
    const std::shared_ptr<FuncPtr> func,
    const std::shared_ptr<List> args,
    Env& env
) {
    return func->get_func()(args, env);
}

std::shared_ptr<Object> apply_part_func(
    const std::shared_ptr<PartiallyAppliedFunction> func,
    const std::shared_ptr<List> args,
    Env& env
) {
    auto new_args = func->get_args();
    new_args->append(args);
    return apply_func(func->get_func(), new_args, env);
}

std::shared_ptr<Object> apply_func(
    const std::shared_ptr<Function> func,
    const std::shared_ptr<List> args,
    Env& env
) {
    std::list<std::shared_ptr<Object>> arg_list;
    auto head = args;
    while (head != nullptr) {
        arg_list.push_back(head->get_value());
        head = head->get_next();
    }

    Env temp_env(env);
    if (arg_list.size() > func->get_params().size()) {
        std::ostringstream ss;
        ss << "different number of argument to function: expect " << func->get_params().size();
        ss << ", but got " << arg_list.size();
        throw EvalException(ss.str());
    } else if (arg_list.size() == func->get_params().size()){
        auto syms = func->get_params().begin();
        auto args = arg_list.begin();
        while (syms != func->get_params().end()) {
            temp_env.set_obj((*syms)->get_symbol(), eval(*args, env));
            syms++; args++;
        }
    } else {
        return std::make_shared<PartiallyAppliedFunction>(func, args);
    }

    std::shared_ptr<Object> result = GLOBAL_NIL;
    for (auto& body : func->get_body()) {
        result = eval(body, temp_env);
    }
    return result;
}

std::shared_ptr<Object> eval_symbol(const std::shared_ptr<Symbol>& symbol, Env& env) {
    return env.get_obj(symbol->get_symbol());
}

std::shared_ptr<Object> fn_quote(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    TAKE_JUST_ONE_ARG("quote", args, a1);

    return a1;
}

std::shared_ptr<Object> fn_list(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_ONE_ARG("list", args, env, a1);

    auto list = std::make_shared<List>(a1);
    auto arg_it = args->get_next();
    while (arg_it != nullptr) {
        // HACK: When we append item to list, it takes O(len(list)).
        //       The length increment if I append item. So, the time complexity is
        //       O(1 + 2 + .. + n) = O(n^2), which is slow if number of item is too big.
        list->append(std::make_shared<List>(eval(arg_it->get_value(), env)));
        arg_it = arg_it->get_next();
    }
    return list;
}

std::shared_ptr<Object> fn_car(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("car", args, env, a1);

    if (a1->kind() == ObjectKind::List) {
        auto list = std::dynamic_pointer_cast<List>(a1);
        return list->get_value();
    } else if (a1->kind() == ObjectKind::NIL) {
        return a1;
    } else {
        throw EvalException(a1->debug() + " is not a list");
    }
}

std::shared_ptr<Object> fn_cdr(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("cdr", args, env, a1);

    if (a1->kind() == ObjectKind::List) {
        auto list = std::dynamic_pointer_cast<List>(a1);
        if (list->get_next() == nullptr) {
            return GLOBAL_NIL;
        } else {
            return list->get_next();
        }
    } else if (a1->kind() == ObjectKind::NIL) {
        return a1;
    } else {
        throw EvalException(a1->debug() + " is not a list");
    }
}

std::shared_ptr<Object> fn_cons(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("cons", args, env, a1, a2);

    if (a2->kind() == ObjectKind::List) {
        return std::make_shared<List>(a1, std::dynamic_pointer_cast<List>(a2));
    } else {
        std::shared_ptr<List> list = std::make_shared<List>(a1);
        list->append(std::make_shared<List>(a2));
        return list;
    }
}

std::shared_ptr<Object> fn_atom(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("atom", args, env, a1);

    if (a1->is_atom()) {
        return GLOBAL_T;
    } else {
        return GLOBAL_NIL;
    }
}

std::shared_ptr<Object> fn_if(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2, a3;
    TAKE_JUST_THREE_ARG("if", args, a1, a2, a3);

    if (eval(a1, env)->kind() != ObjectKind::NIL) {
        return eval(a2, env);
    } else {
        return eval(a3, env);
    }
}

#define APPLY_COMP_OP_TO_NUMS(a1, a2, op) \
    do { \
        if (a1->kind() == ObjectKind::Integer && a2->kind() == ObjectKind::Integer) { \
            int l = std::dynamic_pointer_cast<Integer>(a1)->get_integer(); \
            int r = std::dynamic_pointer_cast<Integer>(a2)->get_integer(); \
            if (l op r) { return GLOBAL_T; } else { return GLOBAL_NIL; } \
        } else if (a1->kind() == ObjectKind::Integer && a2->kind() == ObjectKind::Number) { \
            double l = std::dynamic_pointer_cast<Integer>(a1)->get_integer(); \
            double r = std::dynamic_pointer_cast<Number>(a2)->get_number(); \
            if (l op r) { return GLOBAL_T; } else { return GLOBAL_NIL; } \
        } else if (a1->kind() == ObjectKind::Number && a2->kind() == ObjectKind::Integer) { \
            double l = std::dynamic_pointer_cast<Number>(a1)->get_number(); \
            double r = std::dynamic_pointer_cast<Integer>(a2)->get_integer(); \
            if (l op r) { return GLOBAL_T; } else { return GLOBAL_NIL; } \
        } else if (a1->kind() == ObjectKind::Number && a2->kind() == ObjectKind::Number) { \
            double l = std::dynamic_pointer_cast<Number>(a1)->get_number(); \
            double r = std::dynamic_pointer_cast<Number>(a2)->get_number(); \
            if (l op r) { return GLOBAL_T; } else { return GLOBAL_NIL; } \
        } else { \
            std::ostringstream ss; \
            ss << std::string(#op) << " cannot be applied to non-numeric objects: "; \
            ss << "lhs is " << a1->debug() << " and rhs is " << a2->debug(); \
            throw EvalException(ss.str()); \
        } \
    } while (0)

#define APPLY_ARITH_OP_TO_NUMS(a1, a2, a3, op) \
    do { \
        if (a1->kind() == ObjectKind::Integer && a2->kind() == ObjectKind::Integer) { \
            int l = std::dynamic_pointer_cast<Integer>(a1)->get_integer(); \
            int r = std::dynamic_pointer_cast<Integer>(a2)->get_integer(); \
            a3 = std::make_shared<Integer>(l op r); \
        } else if (a1->kind() == ObjectKind::Integer && a2->kind() == ObjectKind::Number) { \
            double l = std::dynamic_pointer_cast<Integer>(a1)->get_integer(); \
            double r = std::dynamic_pointer_cast<Number>(a2)->get_number(); \
            a3 = std::make_shared<Number>(l op r); \
        } else if (a1->kind() == ObjectKind::Number && a2->kind() == ObjectKind::Integer) { \
            double l = std::dynamic_pointer_cast<Number>(a1)->get_number(); \
            double r = std::dynamic_pointer_cast<Integer>(a2)->get_integer(); \
            a3 = std::make_shared<Number>(l op r); \
        } else if (a1->kind() == ObjectKind::Number && a2->kind() == ObjectKind::Number) { \
            double l = std::dynamic_pointer_cast<Number>(a1)->get_number(); \
            double r = std::dynamic_pointer_cast<Number>(a2)->get_number(); \
            a3 = std::make_shared<Number>(l op r); \
        } else { \
            std::ostringstream ss; \
            ss << std::string(#op) << " cannot be applied to non-numeric objects: "; \
            ss << "lhs is " << a1->debug() << " and rhs is " << a2->debug(); \
            throw EvalException(ss.str()); \
        } \
    } while (0)

std::shared_ptr<Object> fn_eq_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("=", args, env, a1, a2);
    APPLY_COMP_OP_TO_NUMS(a1, a2, ==);
}

std::shared_ptr<Object> fn_ne_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("/=", args, env, a1, a2);
    APPLY_COMP_OP_TO_NUMS(a1, a2, !=);
}

std::shared_ptr<Object> fn_lt_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("<", args, env, a1, a2);
    APPLY_COMP_OP_TO_NUMS(a1, a2, <);
}

std::shared_ptr<Object> fn_gt_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG(">", args, env, a1, a2);
    APPLY_COMP_OP_TO_NUMS(a1, a2, >);
}

std::shared_ptr<Object> fn_le_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("<=", args, env, a1, a2);
    APPLY_COMP_OP_TO_NUMS(a1, a2, <=);
}

std::shared_ptr<Object> fn_ge_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG(">=", args, env, a1, a2);
    APPLY_COMP_OP_TO_NUMS(a1, a2, >=);
}

std::shared_ptr<Object> fn_add_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2, acc;
    EVAL_TWO_ARG("+", args, env, a1, a2);
    APPLY_ARITH_OP_TO_NUMS(a1, a2, acc, +);
    auto head = args->get_next()->get_next();
    while (head != nullptr) {
        std::shared_ptr<Object> a;
        EVAL_ONE_ARG("+", head, env, a);
        APPLY_ARITH_OP_TO_NUMS(a, acc, acc, +);
        head = head->get_next();
    }
    return acc;
}

std::shared_ptr<Object> fn_sub_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2, acc;
    EVAL_TWO_ARG("-", args, env, a1, a2);
    APPLY_ARITH_OP_TO_NUMS(a1, a2, acc, -);
    auto head = args->get_next()->get_next();
    while (head != nullptr) {
        std::shared_ptr<Object> a;
        EVAL_ONE_ARG("-", head, env, a);
        APPLY_ARITH_OP_TO_NUMS(a, acc, acc, -);
        head = head->get_next();
    }
    return acc;
}

std::shared_ptr<Object> fn_mul_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2, acc;
    EVAL_TWO_ARG("*", args, env, a1, a2);
    APPLY_ARITH_OP_TO_NUMS(a1, a2, acc, *);
    auto head = args->get_next()->get_next();
    while (head != nullptr) {
        std::shared_ptr<Object> a;
        EVAL_ONE_ARG("*", head, env, a);
        APPLY_ARITH_OP_TO_NUMS(a, acc, acc, *);
        head = head->get_next();
    }
    return acc;
}

std::shared_ptr<Object> fn_div_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2, acc;
    EVAL_TWO_ARG("/", args, env, a1, a2);
    APPLY_ARITH_OP_TO_NUMS(a1, a2, acc, /);
    auto head = args->get_next()->get_next();
    while (head != nullptr) {
        std::shared_ptr<Object> a;
        EVAL_ONE_ARG("/", head, env, a);
        APPLY_ARITH_OP_TO_NUMS(a, acc, acc, /);
        head = head->get_next();
    }
    return acc;
}

#define APPLY_COMP_OP_TO_STRS(name, a1, a2, op, ignore_upper_lower) \
    do { \
        if (a1->kind() == ObjectKind::String && a2->kind() == ObjectKind::String) { \
            auto l = std::dynamic_pointer_cast<String>(a1)->get_string(); \
            auto r = std::dynamic_pointer_cast<String>(a2)->get_string(); \
            if ((ignore_upper_lower)) { \
                std::transform(l.begin(), l.end(), l.begin(), tolower); \
                std::transform(r.begin(), r.end(), r.begin(), tolower); \
            } \
            if (l op r) { \
                return GLOBAL_T; \
            } else { \
                return GLOBAL_NIL; \
            } \
        } else { \
            std::ostringstream ss; \
            ss << "arguments of " << name << " must be string"; \
            throw EvalException(ss.str()); \
        } \
    } while (0)

std::shared_ptr<Object> fn_eq_str(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("string=", args, env, a1, a2);
    APPLY_COMP_OP_TO_STRS("string=", a1, a2, ==, false);
}

std::shared_ptr<Object> fn_ne_str(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("string/=", args, env, a1, a2);
    APPLY_COMP_OP_TO_STRS("string/=", a1, a2, !=, false);
}

std::shared_ptr<Object> fn_lt_str(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("string<", args, env, a1, a2);
    APPLY_COMP_OP_TO_STRS("string<", a1, a2, <, false);
}

std::shared_ptr<Object> fn_gt_str(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("string>", args, env, a1, a2);
    APPLY_COMP_OP_TO_STRS("string>", a1, a2, >, false);
}

std::shared_ptr<Object> fn_le_str(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("string<=", args, env, a1, a2);
    APPLY_COMP_OP_TO_STRS("string<=", a1, a2, <=, false);
}

std::shared_ptr<Object> fn_ge_str(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("string>=", args, env, a1, a2);
    APPLY_COMP_OP_TO_STRS("string>=", a1, a2, >=, false);
}

std::shared_ptr<Object> fn_equal_str(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("string>=", args, env, a1, a2);
    APPLY_COMP_OP_TO_STRS("string>=", a1, a2, >=, true);
}

std::shared_ptr<Object> fn_write(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("write", args, env, a1);
    if (a1->kind() == ObjectKind::String) {
        std::cout << '"' << std::dynamic_pointer_cast<String>(a1)->get_string() << '"';
    } else if (a1->kind() == ObjectKind::Integer) {
        std::cout << std::dynamic_pointer_cast<Integer>(a1)->get_integer();
    } else if (a1->kind() == ObjectKind::Number) {
        std::cout << std::to_string(std::dynamic_pointer_cast<Number>(a1)->get_number());
    } else {
        throw EvalException("write can only accpet string, integer or number");
    }
    return a1;
}

std::shared_ptr<Object> fn_write_line(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("write-line", args, env, a1);
    if (a1->kind() == ObjectKind::String) {
        std::cout << std::dynamic_pointer_cast<String>(a1)->get_string() << std::endl;
    } else {
        throw EvalException("write-line can only accpet string");
    }
    return a1;
}

std::shared_ptr<Object> fn_print(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("write", args, env, a1);
    if (a1->kind() == ObjectKind::String) {
        std::cout << std::endl << '"' << std::dynamic_pointer_cast<String>(a1)->get_string() << '"';
    } else if (a1->kind() == ObjectKind::Integer) {
        std::cout << std::endl << std::dynamic_pointer_cast<Integer>(a1)->get_integer();
    } else if (a1->kind() == ObjectKind::Number) {
        std::cout << std::endl << std::to_string(std::dynamic_pointer_cast<Number>(a1)->get_number());
    } else {
        throw EvalException("print can only accpet string, integer or number");
    }
    return a1;
}

std::shared_ptr<Object> fn_prin1(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("write", args, env, a1);
    if (a1->kind() == ObjectKind::String) {
        std::cout << '"' << std::dynamic_pointer_cast<String>(a1)->get_string() << '"';
    } else if (a1->kind() == ObjectKind::Integer) {
        std::cout << std::dynamic_pointer_cast<Integer>(a1)->get_integer();
    } else if (a1->kind() == ObjectKind::Number) {
        std::cout << std::to_string(std::dynamic_pointer_cast<Number>(a1)->get_number());
    } else {
        throw EvalException("prin1 can only accpet string, integer or number");
    }
    return a1;
}

std::shared_ptr<Object> fn_princ(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("write", args, env, a1);
    if (a1->kind() == ObjectKind::String) {
        std::cout << std::dynamic_pointer_cast<String>(a1)->get_string();
    } else if (a1->kind() == ObjectKind::Integer) {
        std::cout << std::dynamic_pointer_cast<Integer>(a1)->get_integer();
    } else if (a1->kind() == ObjectKind::Number) {
        std::cout << std::to_string(std::dynamic_pointer_cast<Number>(a1)->get_number());
    } else {
        throw EvalException("princ can only accpet string, integer or number");
    }
    return a1;
}

std::shared_ptr<Object> fn_read_str(const std::shared_ptr<List> args, Env& env) {
    if (args != nullptr) {
        throw EvalException("too many arguments for reads");
    }

    std::string s;
    if (std::cin >> s) {
        return std::make_shared<String>(s);
    } else {
        throw EvalException("faild to read a string");
    }
}

std::shared_ptr<Object> fn_read_int(const std::shared_ptr<List> args, Env& env) {
    if (args != nullptr) {
        throw EvalException("too many arguments for readi");
    }

    int i;
    if (std::cin >> i) {
        return std::make_shared<Integer>(i);
    } else {
        throw EvalException("faild to read an integer");
    }
}

std::shared_ptr<Object> fn_read_num(const std::shared_ptr<List> args, Env& env) {
    if (args != nullptr) {
        throw EvalException("too many arguments for readn");
    }

    double n;
    if (std::cin >> n) {
        return std::make_shared<Number>(n);
    } else {
        throw EvalException("faild to read a number");
    }
}

std::shared_ptr<Object> fn_lambda(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    TAKE_ONE_ARG("lambda", args, a1);

    if (a1->kind() == ObjectKind::List) {
        std::list<std::shared_ptr<Symbol>> lambda_args = {};
        auto head = std::dynamic_pointer_cast<List>(a1);
        while (head != nullptr) {
            auto obj = head->get_value();
            if (obj->kind() != ObjectKind::Symbol) {
                throw EvalException("list elements of lambda must be symbol");
            } else {
                lambda_args.push_back(std::dynamic_pointer_cast<Symbol>(obj));
                head = head->get_next();
            }
        }

        std::list<std::shared_ptr<Object>> lambda_body = {};
        if (args->get_next() != nullptr) {
            lambda_body = args->get_next()->to_list();
        }

        return std::make_shared<Function>(lambda_args, lambda_body);
    } else if (a1->kind() == ObjectKind::NIL) {
        std::list<std::shared_ptr<Object>> lambda_body = {};
        if (args->get_next() != nullptr) {
            lambda_body = args->get_next()->to_list();
        }

        return std::make_shared<Function>(std::list<std::shared_ptr<Symbol>>(), lambda_body);
    } else {
        throw EvalException("first argument of lambda must be list");
    }
}

std::shared_ptr<Object> fn_macro(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    TAKE_ONE_ARG("macro", args, a1);

    if (a1->kind() == ObjectKind::List) {
        std::list<std::shared_ptr<Symbol>> macro_args = {};
        auto head = std::dynamic_pointer_cast<List>(a1);
        while (head != nullptr) {
            auto obj = head->get_value();
            if (obj->kind() != ObjectKind::Symbol) {
                throw EvalException("list elements of macro must be symbol");
            } else {
                macro_args.push_back(std::dynamic_pointer_cast<Symbol>(obj));
                head = head->get_next();
            }
        }

        std::list<std::shared_ptr<Object>> macro_body = {};
        if (args->get_next() != nullptr) {
            macro_body = args->get_next()->to_list();
        }

        return std::make_shared<Macro>(macro_args, macro_body);
    } else if (a1->kind() == ObjectKind::NIL) {
        std::list<std::shared_ptr<Object>> macro_body = {};
        if (args->get_next() != nullptr) {
            macro_body = args->get_next()->to_list();
        }

        return std::make_shared<Macro>(std::list<std::shared_ptr<Symbol>>(), macro_body);
    } else {
        throw EvalException("first argument of macro must be list");
    }
}

std::shared_ptr<Object> fn_set(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_JUST_TWO_ARG("set", args, env, a1, a2);

    if (a1->kind() != ObjectKind::Symbol) {
        throw EvalException("first argument of set must have symbol");
    }
    auto name = std::dynamic_pointer_cast<Symbol>(a1)->get_symbol();
    env.set_obj(name, a2);
    return a2;
}

std::shared_ptr<Object> fn_setq(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    TAKE_JUST_TWO_ARG("setq", args, a1, a2);

    if (a1->kind() != ObjectKind::Symbol) {
        throw EvalException("first argument of setq must be symbol");
    }
    auto name = std::dynamic_pointer_cast<Symbol>(a1)->get_symbol();
    env.set_obj(name, eval(a2, env));
    return a2;
}

std::shared_ptr<Object> fn_int_to_string(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("int-to-string", args, env, a1);

    if (a1->kind() == ObjectKind::Integer) {
        auto integer = std::dynamic_pointer_cast<Integer>(a1)->get_integer();
        return std::make_shared<String>(std::to_string(integer));
    } else {
        throw EvalException("given object is not an integer");
    }
}

std::shared_ptr<Object> fn_num_to_string(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("num-to-string", args, env, a1);

    if (a1->kind() == ObjectKind::Number) {
        auto number = std::dynamic_pointer_cast<Number>(a1)->get_number();
        return std::make_shared<String>(std::to_string(number));
    } else {
        throw EvalException("given object is not a number");
    }
}

std::shared_ptr<Object> fn_debug(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("debug", args, env, a1);
    return std::make_shared<String>(a1->debug());
}

std::shared_ptr<Object> fn_type_of(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_JUST_ONE_ARG("type-of", args, env, a1);

    switch (a1->kind()) {
        case ObjectKind::List:
            return std::make_shared<String>("List");
        case ObjectKind::T:
            return std::make_shared<String>("T");
        case ObjectKind::NIL:
            return std::make_shared<String>("NIL");
        case ObjectKind::Integer:
            return std::make_shared<String>("Integer");
        case ObjectKind::Number:
            return std::make_shared<String>("Number");
        case ObjectKind::String:
            return std::make_shared<String>("String");
        case ObjectKind::Symbol:
            return std::make_shared<String>("Symbol");
        case ObjectKind::Function:
            return std::make_shared<String>("Function");
        case ObjectKind::FuncPtr:
            return std::make_shared<String>("FuncPtr");
        case ObjectKind::PartiallyAppliedFunction:
            return std::make_shared<String>("PartiallyAppliedFunction");
        case ObjectKind::PartiallyAppliedFuncPtr:
            return std::make_shared<String>("PartiallyAppliedFuncPtr");
        default:
            throw EvalException("unreachable");
    }
}

std::shared_ptr<Object> fn_concat(const std::shared_ptr<List> args, Env& env) {
    std::string acc;
    std::shared_ptr<Object> a1, a2;
    EVAL_TWO_ARG("concat", args, env, a1, a2);
    if (a1->kind() == ObjectKind::String && a2->kind() == ObjectKind::String) {
        acc = std::dynamic_pointer_cast<String>(a1)->get_string() +
              std::dynamic_pointer_cast<String>(a2)->get_string();
    } else {
        throw EvalException("arguments of concat must be string");
    }
    auto head = args->get_next()->get_next();
    while (head != nullptr) {
        std::shared_ptr<Object> a;
        EVAL_ONE_ARG("concat", head, env, a);
        if (a->kind() == ObjectKind::String) {
            acc += std::dynamic_pointer_cast<String>(a)->get_string();
        } else {
            throw EvalException("arguments of concat must be string");
        }
        head = head->get_next();
    }
    return std::make_shared<String>(acc);
}

std::shared_ptr<Object> fn_defun(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    TAKE_ONE_ARG("defun", args, a1);

    if (a1->kind() != ObjectKind::Symbol) {
        throw EvalException("first argument of defun must be symbol");
    }

    auto lambda = fn_lambda(args->get_next(), env);
    env.set_obj(std::dynamic_pointer_cast<Symbol>(a1)->get_symbol(), lambda);
    return lambda;
}

std::shared_ptr<Object> fn_defmacro(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    TAKE_ONE_ARG("defun", args, a1);

    if (a1->kind() != ObjectKind::Symbol) {
        throw EvalException("first argument of defmacro must be symbol");
    }

    auto macro = fn_macro(args->get_next(), env);
    env.set_obj(std::dynamic_pointer_cast<Symbol>(a1)->get_symbol(), macro);
    return macro;
}

std::shared_ptr<Object> fn_macroexpand(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_ONE_ARG("macroexpand", args, env, a1);

    if (a1->kind() != ObjectKind::List) {
        throw EvalException("first argument of macroexpand must be evaluated to list");
    }

    auto list = std::dynamic_pointer_cast<List>(a1);
    if (list->get_value()->kind() == ObjectKind::Symbol) {
        auto macro_name = std::dynamic_pointer_cast<Symbol>(list->get_value())->get_symbol();
        auto maybe_macro = env.get_obj(macro_name);
        if (maybe_macro->kind() != ObjectKind::Macro) {
            throw EvalException("first element of list must hold macro");
        }
        auto macro = std::dynamic_pointer_cast<Macro>(maybe_macro);
        auto expanded = expand_macro(macro, list->get_next(), env);
        if (expanded.empty()) {
            return GLOBAL_NIL;
        } else {
            return *expanded.rbegin();
        }
    } else if (list->get_value()->kind() == ObjectKind::Macro) {
        auto macro = std::dynamic_pointer_cast<Macro>(list->get_next());
        auto expanded = expand_macro(macro, list->get_next(), env);
        if (expanded.empty()) {
            return GLOBAL_NIL;
        } else {
            return *expanded.rbegin();
        }
    } else {
        throw EvalException("first element of list must be symbol or macro");
    }
}

Env default_env() {
    Env env;
    env.set_obj("quote", std::make_shared<FuncPtr>(fn_quote));
    env.set_obj("list", std::make_shared<FuncPtr>(fn_list));
    env.set_obj("car", std::make_shared<FuncPtr>(fn_car));
    env.set_obj("cdr", std::make_shared<FuncPtr>(fn_cdr));
    env.set_obj("cons", std::make_shared<FuncPtr>(fn_cons));
    env.set_obj("atom", std::make_shared<FuncPtr>(fn_atom));
    env.set_obj("if", std::make_shared<FuncPtr>(fn_if));
    env.set_obj("=", std::make_shared<FuncPtr>(fn_eq_num));
    env.set_obj("/=", std::make_shared<FuncPtr>(fn_ne_num));
    env.set_obj("<", std::make_shared<FuncPtr>(fn_lt_num));
    env.set_obj(">", std::make_shared<FuncPtr>(fn_gt_num));
    env.set_obj("<=", std::make_shared<FuncPtr>(fn_le_num));
    env.set_obj(">=", std::make_shared<FuncPtr>(fn_ge_num));
    env.set_obj("+", std::make_shared<FuncPtr>(fn_add_num));
    env.set_obj("-", std::make_shared<FuncPtr>(fn_sub_num));
    env.set_obj("*", std::make_shared<FuncPtr>(fn_mul_num));
    env.set_obj("/", std::make_shared<FuncPtr>(fn_div_num));
    env.set_obj("string=", std::make_shared<FuncPtr>(fn_eq_str));
    env.set_obj("string/=", std::make_shared<FuncPtr>(fn_ne_str));
    env.set_obj("string<", std::make_shared<FuncPtr>(fn_lt_str));
    env.set_obj("string>", std::make_shared<FuncPtr>(fn_gt_str));
    env.set_obj("string<=", std::make_shared<FuncPtr>(fn_le_str));
    env.set_obj("string>=", std::make_shared<FuncPtr>(fn_ge_str));
    env.set_obj("string-equal", std::make_shared<FuncPtr>(fn_equal_str));
    env.set_obj("write", std::make_shared<FuncPtr>(fn_write));
    env.set_obj("write-line", std::make_shared<FuncPtr>(fn_write_line));
    env.set_obj("print", std::make_shared<FuncPtr>(fn_print));
    env.set_obj("prin1", std::make_shared<FuncPtr>(fn_prin1));
    env.set_obj("princ", std::make_shared<FuncPtr>(fn_princ));
    env.set_obj("read-str", std::make_shared<FuncPtr>(fn_read_str));
    env.set_obj("read-int", std::make_shared<FuncPtr>(fn_read_int));
    env.set_obj("read-num", std::make_shared<FuncPtr>(fn_read_num));
    env.set_obj("lambda", std::make_shared<FuncPtr>(fn_lambda));
    env.set_obj("macro", std::make_shared<FuncPtr>(fn_macro));
    env.set_obj("set", std::make_shared<FuncPtr>(fn_set));
    env.set_obj("setq", std::make_shared<FuncPtr>(fn_setq));
    env.set_obj("int-to-string", std::make_shared<FuncPtr>(fn_int_to_string));
    env.set_obj("num-to-string", std::make_shared<FuncPtr>(fn_num_to_string));
    env.set_obj("debug", std::make_shared<FuncPtr>(fn_debug));
    env.set_obj("type-of", std::make_shared<FuncPtr>(fn_type_of));
    env.set_obj("concat", std::make_shared<FuncPtr>(fn_concat));
    env.set_obj("defun", std::make_shared<FuncPtr>(fn_defun));
    env.set_obj("defmacro", std::make_shared<FuncPtr>(fn_defmacro));
    env.set_obj("macroexpand", std::make_shared<FuncPtr>(fn_macroexpand));
    env.set_obj("T", GLOBAL_T);
    env.set_obj("NIL", GLOBAL_NIL);
    return env;
}

std::istream& prompt(std::istream& is, const std::string& msg, std::string& input) {
    std::cout << msg << ": " << std::flush;
    return std::getline(is, input);
}

void interpreter(Env& env) {
    std::string input;
    std::cout << "press CTRL-D to exit from this interpreter" << std::endl;
    while (prompt(std::cin, "input", input)) {
        try {
            auto tokens = lex(input);
            auto objs = parse(tokens);
            for (const auto& obj : objs) {
                std::cout << eval(obj, env)->debug() << std::endl;
            }
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

void run(std::string input, Env& env) {
    try {
        for (const auto& obj : parse(lex(input))) {
            eval(obj, env);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char *argv[]) {
    Env env = default_env();
    if (argc == 2) {
        std::ifstream ifs(argv[1]);
        if (!ifs) {
            std::cerr << "faild to open file " << argv[1] << std::endl;
            std::exit(1);
        }
        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        run(content, env);
    } else {
        interpreter(env);
    }
}
