#include <cassert>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <istream>
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

std::vector<std::shared_ptr<Token>> lex(const std::string& input) {
    std::vector<std::shared_ptr<Token>> tokens;
    auto it = input.begin();
    const auto last = input.end();
    while (true) {
        while (it != last && isspace(*it)) { it++; }
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

    // Objects which internally used and user can't create these object.
    FuncPtr,
};

class Object {
public:
    virtual ~Object() {}
    virtual ObjectKind kind() const = 0;
    virtual bool is_atom() const = 0;
    virtual std::string debug() const = 0;
};

// This use `Object` and `FuncPtr` use this, so this must be placed between `Object` and `FuncPtr`.
class Env {
private:
    std::map<std::string, std::shared_ptr<Object>> symtable;

public:
    std::shared_ptr<Object> get_obj(const std::string& sym) {
        try {
            return symtable.at(sym);
        } catch (std::out_of_range& _) {
            std::cout << "no such symbol exist: " << sym << std::endl;
            exit(1);
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

    void append(std::shared_ptr<Object> obj) {
        auto list = shared_from_this();
        while (list->next != nullptr) {
            list = list->next;
        }
        list->next = std::make_shared<List>(obj);
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
                list->append(parse_object(it, last));
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

#define TAKE_ONE_ARG(name, args, a1) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for" + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() != nullptr) { \
            throw EvalException("too many arguments for" + std::string(name)); \
        } \
    } while (0)

#define TAKE_TWO_ARG(name, args, a1, a2) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for" + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() == nullptr) { \
            throw EvalException("too few arguments for" + std::string(name)); \
        } \
        a2 = args->get_next()->get_value(); \
        if (args->get_next()->get_next() != nullptr) { \
            throw EvalException("too many arguments for" + std::string(name)); \
        } \
    } while (0)

#define TAKE_THREE_ARG(name, args, a1, a2, a3) \
    do { \
        if (args == nullptr) { \
            throw EvalException("too few arguments for" + std::string(name)); \
        } \
        a1 = args->get_value(); \
        if (args->get_next() == nullptr) { \
            throw EvalException("too few arguments for" + std::string(name)); \
        } \
        a2 = args->get_next()->get_value(); \
        if (args->get_next()->get_next() == nullptr) { \
            throw EvalException("too few arguments for" + std::string(name)); \
        } \
        a3 = args->get_next()->get_next()->get_value(); \
        if (args->get_next()->get_next()->get_next() != nullptr) { \
            throw EvalException("too many arguments for" + std::string(name)); \
        } \
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
std::shared_ptr<Object> call(const std::string& name, const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_quote(const std::shared_ptr<List> args, Env& env);
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
std::shared_ptr<Object> fn_write(const std::shared_ptr<List> args, Env& env);
std::shared_ptr<Object> fn_write_line(const std::shared_ptr<List> args, Env& env);

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
        return call(name, list->get_next(), env);
    } else {
        throw EvalException("first object of list must be symbol");
    }
}

std::shared_ptr<Object> eval_symbol(const std::shared_ptr<Symbol>& symbol, Env& env) {
    return env.get_obj(symbol->get_symbol());
}

std::shared_ptr<Object> call(const std::string& name, const std::shared_ptr<List> args, Env& env) {
    auto obj = env.get_obj(name);
    if (obj->kind() == ObjectKind::FuncPtr) {
        return std::dynamic_pointer_cast<FuncPtr>(obj)->get_func()(args, env);
    } else {
        throw EvalException(name + " is not a function");
    }
}

std::shared_ptr<Object> fn_quote(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    TAKE_ONE_ARG("quote", args, a1);

    return a1;
}

std::shared_ptr<Object> fn_car(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_ONE_ARG("car", args, env, a1);

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
    EVAL_ONE_ARG("cdr", args, env, a1);

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
    EVAL_TWO_ARG("cons", args, env, a1, a2);

    if (a2->kind() == ObjectKind::List) {
        return std::make_shared<List>(a1, std::dynamic_pointer_cast<List>(a2));
    } else {
        std::shared_ptr<List> list = std::make_shared<List>(a1);
        list->append(a2);
        return list;
    }
}

std::shared_ptr<Object> fn_atom(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_ONE_ARG("atom", args, env, a1);

    if (a1->is_atom()) {
        return GLOBAL_T;
    } else {
        return GLOBAL_NIL;
    }
}

std::shared_ptr<Object> fn_if(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2, a3;
    TAKE_THREE_ARG("if", args, a1, a2, a3);

    if (eval(a1, env)->kind() != ObjectKind::NIL) {
        return eval(a2, env);
    } else {
        return eval(a3, env);
    }
}

#define APPLY_OP_TO_NUMS(a1, a2, op) \
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
            throw EvalException(std::string(#op) + "cannot be applied to non-numeric object"); \
        } \
    } while (0)

std::shared_ptr<Object> fn_eq_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_TWO_ARG("=", args, env, a1, a2);
    APPLY_OP_TO_NUMS(a1, a2, ==);
}

std::shared_ptr<Object> fn_ne_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_TWO_ARG("=", args, env, a1, a2);
    APPLY_OP_TO_NUMS(a1, a2, !=);
}

std::shared_ptr<Object> fn_lt_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_TWO_ARG("=", args, env, a1, a2);
    APPLY_OP_TO_NUMS(a1, a2, <);
}

std::shared_ptr<Object> fn_gt_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_TWO_ARG("=", args, env, a1, a2);
    APPLY_OP_TO_NUMS(a1, a2, >);
}

std::shared_ptr<Object> fn_le_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_TWO_ARG("=", args, env, a1, a2);
    APPLY_OP_TO_NUMS(a1, a2, <=);
}

std::shared_ptr<Object> fn_ge_num(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1, a2;
    EVAL_TWO_ARG("=", args, env, a1, a2);
    APPLY_OP_TO_NUMS(a1, a2, >=);
}

std::string remove_tail_zeros(const std::string& s) {
    auto it = s.rend();
    while (*it == '0' && *(it + 1) != '.') { it++; }
    return std::string(s.begin(), it.base());
}

std::shared_ptr<Object> fn_write(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_ONE_ARG("write", args, env, a1);
    if (a1->kind() == ObjectKind::String) {
        std::cout << '"' << std::dynamic_pointer_cast<String>(a1)->get_string() << '"';
    } else if (a1->kind() == ObjectKind::Integer) {
        std::cout << std::dynamic_pointer_cast<Integer>(a1)->get_integer();
    } else if (a1->kind() == ObjectKind::Number) {
        std::string s = std::to_string(std::dynamic_pointer_cast<Number>(a1)->get_number());
        std::cout << remove_tail_zeros(s);
    } else {
        throw EvalException("write can only accpet string, integer or number");
    }
    return a1;
}

std::shared_ptr<Object> fn_write_line(const std::shared_ptr<List> args, Env& env) {
    std::shared_ptr<Object> a1;
    EVAL_ONE_ARG("write", args, env, a1);
    if (a1->kind() == ObjectKind::String) {
        std::cout << '"' << std::dynamic_pointer_cast<String>(a1)->get_string() << '"' << std::endl;
    } else if (a1->kind() == ObjectKind::Integer) {
        std::cout << std::dynamic_pointer_cast<Integer>(a1)->get_integer() << std::endl;
    } else if (a1->kind() == ObjectKind::Number) {
        std::string s = std::to_string(std::dynamic_pointer_cast<Number>(a1)->get_number());
        std::cout << remove_tail_zeros(s) << std::endl;
    } else {
        throw EvalException("write-line can only accpet string, integer or number");
    }
    return a1;
}

Env default_env() {
    Env env;
    env.set_obj("quote", std::make_shared<FuncPtr>(fn_quote));
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
    env.set_obj("write", std::make_shared<FuncPtr>(fn_write));
    env.set_obj("write-line", std::make_shared<FuncPtr>(fn_write_line));
    env.set_obj("T", GLOBAL_T);
    env.set_obj("NIL", GLOBAL_NIL);
    return env;
}

std::istream& prompt(std::istream& is, const std::string& msg, std::string& input) {
    std::cout << msg << ": " << std::flush;
    return std::getline(is, input);
}

int main() {
    std::string input;
    Env env = default_env();
    std::cout << "press CTRL-D to exit from this interpreter" << std::endl;
    while (prompt(std::cin, "input", input)) {
        try {
            auto tokens = lex(input);
            auto atoms = parse(tokens);
            std::cout << eval(atoms.at(0), env)->debug() << std::endl;
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}
