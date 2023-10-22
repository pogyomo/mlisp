#include "env.hpp"
#include "eval.hpp"
#include "object.hpp"
#include <memory>
#include <sstream>
#include <stdexcept>
#include <iostream>

#define APPLY_COMP_OP_TO_NUMS(a1, a2, op) \
    do { \
        auto t = std::make_shared<TObject>(); \
        auto nil = std::make_shared<TObject>(); \
        if ((a1)->kind() == ObjectKind::Integer && (a2)->kind() == ObjectKind::Integer) { \
            int l = std::static_pointer_cast<IntegerObject>(a1)->integer(); \
            int r = std::static_pointer_cast<IntegerObject>(a2)->integer(); \
            if (l op r) { return t; } else { return nil; } \
        } else if ((a1)->kind() == ObjectKind::Integer && (a2)->kind() == ObjectKind::Number) { \
            double l = std::static_pointer_cast<IntegerObject>(a1)->integer(); \
            double r = std::static_pointer_cast<NumberObject>(a2)->number(); \
            if (l op r) { return t; } else { return nil; } \
        } else if ((a1)->kind() == ObjectKind::Number && (a2)->kind() == ObjectKind::Integer) { \
            double l = std::static_pointer_cast<NumberObject>(a1)->number(); \
            double r = std::static_pointer_cast<IntegerObject>(a2)->integer(); \
            if (l op r) { return t; } else { return nil; } \
        } else if ((a1)->kind() == ObjectKind::Number && (a2)->kind() == ObjectKind::Number) { \
            double l = std::static_pointer_cast<NumberObject>(a1)->number(); \
            double r = std::static_pointer_cast<NumberObject>(a2)->number(); \
            if (l op r) { return t; } else { return nil; } \
        } else { \
            std::ostringstream ss; \
            ss << std::string(#op) << " cannot be applied to non-numeric objects: "; \
            ss << "lhs is " << (a1)->debug() << " and rhs is " << (a2)->debug(); \
            throw EvalException(ss.str()); \
        } \
    } while (0)

#define APPLY_ARITH_OP_TO_NUMS(a1, a2, a3, op) \
    do { \
        if ((a1)->kind() == ObjectKind::Integer && (a2)->kind() == ObjectKind::Integer) { \
            int l = std::static_pointer_cast<IntegerObject>(a1)->integer(); \
            int r = std::static_pointer_cast<IntegerObject>(a2)->integer(); \
            a3 = std::make_shared<IntegerObject>(l op r); \
        } else if ((a1)->kind() == ObjectKind::Integer && (a2)->kind() == ObjectKind::Number) { \
            double l = std::static_pointer_cast<IntegerObject>(a1)->integer(); \
            double r = std::static_pointer_cast<NumberObject>(a2)->number(); \
            a3 = std::make_shared<NumberObject>(l op r); \
        } else if ((a1)->kind() == ObjectKind::Number && (a2)->kind() == ObjectKind::Integer) { \
            double l = std::static_pointer_cast<NumberObject>(a1)->number(); \
            double r = std::static_pointer_cast<IntegerObject>(a2)->integer(); \
            a3 = std::make_shared<NumberObject>(l op r); \
        } else if ((a1)->kind() == ObjectKind::Number && (a2)->kind() == ObjectKind::Number) { \
            double l = std::static_pointer_cast<NumberObject>(a1)->number(); \
            double r = std::static_pointer_cast<NumberObject>(a2)->number(); \
            a3 = std::make_shared<NumberObject>(l op r); \
        } else { \
            std::ostringstream ss; \
            ss << std::string(#op) << " cannot be applied to non-numeric objects: "; \
            ss << "lhs is " << (a1)->debug() << " and rhs is " << (a2)->debug(); \
            throw EvalException(ss.str()); \
        } \
    } while (0)

EvalException::EvalException(const std::string& msg) : std::runtime_error(msg) {}

// Take `num` arguments from `args`, then return these arguments.
// If `num < 0`, take arguments as many as possible.
// If `restrict` is true and number of element in `args` is not equal to `num`, throw exception.
std::vector<std::shared_ptr<Object>> take_args(
    const std::string& name,
    std::shared_ptr<ListObject> args,
    int num,
    bool restrict
) {
    if (num < 0) {
        std::vector<std::shared_ptr<Object>> result;
        auto arg = args;
        while (arg != nullptr) {
            result.push_back(arg->value());
            arg = arg->next();
        }
        return result;
    }

    std::vector<std::shared_ptr<Object>> result;
    auto arg = args;
    for (int i = 0; i < num; i++) {
        if (arg == nullptr) {
            throw EvalException("too few arguments for " + name);
        }
        result.push_back(arg->value());
        arg = arg->next();
    }
    if (restrict && arg != nullptr) {
        throw EvalException("too many arguments for " + name);
    }
    return result;
}

// Take `num` arguments from `args`, then return evaluated arguments.
// If `num < 0`, take arguments as many as possible.
// If `restrict` is true and number of element in `args` is not equal to `num`, throw exception.
std::vector<std::shared_ptr<Object>> eval_args(
    const std::string& name,
    std::shared_ptr<ListObject> args,
    Env& env,
    int num,
    bool restrict
) {
    if (num < 0) {
        std::vector<std::shared_ptr<Object>> result;
        auto arg = args;
        while (arg != nullptr) {
            result.push_back(eval(arg->value(), env));
            arg = arg->next();
        }
        return result;
    }

    std::vector<std::shared_ptr<Object>> result;
    auto arg = args;
    for (int i = 0; i < num; i++) {
        if (arg == nullptr) {
            throw EvalException("too few arguments for " + name);
        }
        result.push_back(eval(arg->value(), env));
        arg = arg->next();
    }
    if (restrict && arg != nullptr) {
        throw EvalException("too many arguments for " + name);
    }
    return result;
}

std::shared_ptr<Object> eval(std::shared_ptr<Object> obj, Env& env) {
    auto kind = obj->kind();
    if (kind == ObjectKind::List) {
        return eval_list(std::static_pointer_cast<ListObject>(obj), env);
    } else if (
        kind == ObjectKind::T || kind == ObjectKind::NIL || kind == ObjectKind::Integer ||
        kind == ObjectKind::Number || kind == ObjectKind::String || kind == ObjectKind::Function ||
        kind == ObjectKind::PartiallyAppliedFunction || kind == ObjectKind::Macro ||
        kind == ObjectKind::FuncPtr || kind == ObjectKind::PartiallyAppliedFuncPtr
    ) {
        return obj;
    } else if (kind == ObjectKind::Symbol) {
        auto symbol = std::static_pointer_cast<SymbolObject>(obj)->symbol();
        return env.get_obj(symbol);
    } else if (kind == ObjectKind::Quoted) {
        auto object = std::static_pointer_cast<QuotedObject>(obj)->object();
        return object;
    } else if (kind == ObjectKind::BackQuoted) {
        auto object = std::static_pointer_cast<QuotedObject>(obj)->object();
        return object;
    } else if (kind == ObjectKind::Comma || kind == ObjectKind::CommaAtmark) {
        throw EvalException("comma is illegal outside of backquote");
    } else {
        throw EvalException("unreachable");
    }
}

std::shared_ptr<Object> eval_list(std::shared_ptr<ListObject> list, Env& env) {
    auto first = eval(list->value(), env);
    if (first->kind() == ObjectKind::Function) {
        auto func = std::static_pointer_cast<FunctionObject>(first);
        return apply_func(func, list->next(), env);
    } else if (first->kind() == ObjectKind::PartiallyAppliedFunction) {
        auto func = std::static_pointer_cast<PartiallyAppliedFunctionObject>(first);
        auto args = func->args();
        if (list->next() != nullptr) {
            args->append(list->next());
        }
        return apply_func(func->func(), args, env);
    } else if (first->kind() == ObjectKind::FuncPtr) {
        auto func = std::static_pointer_cast<FuncPtrObject>(first);
        return apply_func_ptr(func, list->next(), env);
    } else if (first->kind() == ObjectKind::PartiallyAppliedFuncPtr) {
        auto func = std::static_pointer_cast<PartiallyAppliedFuncPtrObject>(first);
        auto args = func->args();
        if (list->next() != nullptr) {
            args->append(list->next());
        }
        return apply_func_ptr(func->func(), args, env);
    } else if (first->kind() == ObjectKind::Macro) {
        auto macro = std::static_pointer_cast<MacroObject>(first);
        return apply_macro(macro, list->next(), env);
    } else {
        throw EvalException("first element of list must be evaluated to callable");
    }
}

std::shared_ptr<Object> apply_func(
    std::shared_ptr<FunctionObject> func,
    std::shared_ptr<ListObject> args,
    Env& env
) {
    if (func->params()->kind() == ObjectKind::NIL) {
        if (args->kind() == ObjectKind::NIL) {
            std::shared_ptr<Object> result = std::make_shared<NILObject>();
            auto body = func->body();
            while (body != nullptr) {
                result = eval(body->value(), env);
            }
            return result;
        } else {
            int arg_len = std::static_pointer_cast<ListObject>(args)->size();
            std::ostringstream ss;
            ss << "invalid number of argument for function: expected 0, but got " << arg_len;
            throw EvalException(ss.str());
        }
    } else {
        if (args->kind() == ObjectKind::NIL) {
            int param_len = std::static_pointer_cast<ListObject>(func->params())->size();
            std::ostringstream ss;
            ss << "invalid number of argument for function: expected " << param_len << "but got 0";
            throw EvalException(ss.str());
        } else {
            auto param = std::static_pointer_cast<ListObject>(func->params());
            auto arg = std::static_pointer_cast<ListObject>(args);
            Env temp_env(env);
            while (param != nullptr) {
                auto symbol = std::static_pointer_cast<SymbolObject>(param->value())->symbol();
                temp_env.set_obj(symbol, arg->value());
                param = param->next();
                arg = arg->next();
            }

            std::shared_ptr<Object> result = std::make_shared<NILObject>();
            auto body = func->body();
            while (body != nullptr) {
                result = eval(body->value(), temp_env);
            }
            return result;
        }
    }
}

std::shared_ptr<Object> apply_func_ptr(
    std::shared_ptr<FuncPtrObject> func,
    std::shared_ptr<ListObject> args,
    Env& env
) {
    auto obj = func->func()(args, env);
    return obj;
}

std::shared_ptr<Object> apply_macro(
    std::shared_ptr<MacroObject> macro,
    std::shared_ptr<ListObject> args,
    Env& env
) {
    return eval(expand_macro(macro, args, env), env);
}

std::shared_ptr<Object> expand_macro(
    std::shared_ptr<MacroObject> macro,
    std::shared_ptr<ListObject> args,
    Env& env
) {
    if (macro->params()->kind() == ObjectKind::NIL) {
        if (args->kind() == ObjectKind::NIL) {
            std::shared_ptr<Object> result = std::make_shared<NILObject>();
            auto body = macro->body();
            while (body != nullptr) {
                result = eval(body->value(), env);
                body = body->next();
            }
            return result;
        } else {
            int arg_len = std::static_pointer_cast<ListObject>(args)->size();
            std::ostringstream ss;
            ss << "invalid number of argument for macro: expected 0, but got " << arg_len;
            throw EvalException(ss.str());
        }
    } else {
        if (args->kind() == ObjectKind::NIL) {
            int param_len = std::static_pointer_cast<ListObject>(macro->params())->size();
            std::ostringstream ss;
            ss << "invalid number of argument for macro: expected " << param_len << "but got 0";
            throw EvalException(ss.str());
        } else {
            auto param = std::static_pointer_cast<ListObject>(macro->params());
            auto arg = std::static_pointer_cast<ListObject>(args);
            Env temp_env(env);
            while (param != nullptr) {
                auto symbol = std::static_pointer_cast<SymbolObject>(param->value())->symbol();
                temp_env.set_obj(symbol, arg->value());
                param = param->next();
                arg = arg->next();
            }

            std::shared_ptr<Object> result = std::make_shared<NILObject>();
            auto body = macro->body();
            while (body != nullptr) {
                result = eval(body->value(), temp_env);
                body = body->next();
            }
            return result;
        }
    }
}

std::shared_ptr<Object> fn_eq_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("=", args, env, 2, true);
    APPLY_COMP_OP_TO_NUMS(a[0], a[1], ==);
}

std::shared_ptr<Object> fn_ne_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("/=", args, env, 2, true);
    APPLY_COMP_OP_TO_NUMS(a[0], a[1], !=);
}

std::shared_ptr<Object> fn_lt_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("<", args, env, 2, true);
    APPLY_COMP_OP_TO_NUMS(a[0], a[1], <);
}

std::shared_ptr<Object> fn_gt_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args(">", args, env, 2, true);
    APPLY_COMP_OP_TO_NUMS(a[0], a[1], >);
}

std::shared_ptr<Object> fn_le_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("<=", args, env, 2, true);
    APPLY_COMP_OP_TO_NUMS(a[0], a[1], <=);
}

std::shared_ptr<Object> fn_ge_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args(">=", args, env, 2, true);
    APPLY_COMP_OP_TO_NUMS(a[0], a[1], >=);
}

std::shared_ptr<Object> fn_add_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("+", args, env, -1, true);
    if (a.size() < 2) {
        throw EvalException("too few arguments for +");
    }
    std::shared_ptr<Object> acc;
    APPLY_ARITH_OP_TO_NUMS(a[0], a[1], acc, +);
    for (auto it = a.begin() + 2; it != a.end(); it++) {
        APPLY_ARITH_OP_TO_NUMS(*it, acc, acc, +);
    }
    return acc;
}

std::shared_ptr<Object> fn_sub_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("-", args, env, -1, true);
    if (a.size() < 2) {
        throw EvalException("too few arguments for -");
    }
    std::shared_ptr<Object> acc;
    APPLY_ARITH_OP_TO_NUMS(a[0], a[1], acc, -);
    for (auto it = a.begin() + 2; it != a.end(); it++) {
        APPLY_ARITH_OP_TO_NUMS(*it, acc, acc, -);
    }
    return acc;
}

std::shared_ptr<Object> fn_mul_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("*", args, env, -1, true);
    if (a.size() < 2) {
        throw EvalException("too few arguments for *");
    }
    std::shared_ptr<Object> acc;
    APPLY_ARITH_OP_TO_NUMS(a[0], a[1], acc, *);
    for (auto it = a.begin() + 2; it != a.end(); it++) {
        APPLY_ARITH_OP_TO_NUMS(*it, acc, acc, *);
    }
    return acc;
}

std::shared_ptr<Object> fn_div_num(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("/", args, env, -1, true);
    if (a.size() < 2) {
        throw EvalException("too few arguments for /");
    }
    std::shared_ptr<Object> acc;
    APPLY_ARITH_OP_TO_NUMS(a[0], a[1], acc, /);
    for (auto it = a.begin() + 2; it != a.end(); it++) {
        APPLY_ARITH_OP_TO_NUMS(*it, acc, acc, /);
    }
    return acc;
}

std::shared_ptr<Object> fn_list(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("lambda", args, env, -1, false);
    if (a.empty()) {
        return std::make_shared<NILObject>();
    } else {
        auto it = a.begin();
        auto list = std::make_shared<ListObject>(*it++);
        while (it != a.end()) {
            list->append(*it++);
        }
        return list;
    }
}

std::shared_ptr<Object> fn_lambda(std::shared_ptr<ListObject> args, Env& env) {
    auto a = take_args("lambda", args, 1, false);

    if (a[0]->kind() == ObjectKind::List) {
        auto params = std::static_pointer_cast<ListObject>(a[0]);
        auto param = params;
        while (param != nullptr) {
            if (param->value()->kind() != ObjectKind::Symbol) {
                throw EvalException("lambda parametor must be symbol");
            }
            param = param->next();
        }
        return std::make_shared<FunctionObject>(params, args->next());
    } else if (a[0]->kind() == ObjectKind::NIL) {
        return std::make_shared<FunctionObject>(a[1], args->next());
    } else {
        throw EvalException("second argument of lambda must be list");
    }
}

std::shared_ptr<Object> fn_macro(std::shared_ptr<ListObject> args, Env& env) {
    auto a = take_args("macro", args, 1, false);

    if (a[0]->kind() == ObjectKind::List) {
        auto params = std::static_pointer_cast<ListObject>(a[0]);
        auto param = params;
        while (param != nullptr) {
            if (param->value()->kind() != ObjectKind::Symbol) {
                throw EvalException("macro parametor must be symbol");
            }
            param = param->next();
        }
        return std::make_shared<MacroObject>(params, args->next());
    } else if (a[0]->kind() == ObjectKind::NIL) {
        return std::make_shared<MacroObject>(a[1], args->next());
    } else {
        throw EvalException("second argument of macro must be list");
    }
}

std::shared_ptr<Object> fn_macroexpand(std::shared_ptr<ListObject> args, Env& env) {
    auto a = eval_args("macro", args, env, 1, true);

    if (a[0]->kind() != ObjectKind::List) {
        throw EvalException("first argument of macroexpand must be evaluated to list");
    }

    auto list = std::static_pointer_cast<ListObject>(a[0]);
    auto first = eval(list->value(), env);
    if (first->kind() != ObjectKind::Macro) {
        throw EvalException("first element of list must be evaluated to macro");
    }
    auto macro = std::static_pointer_cast<MacroObject>(first);
    return expand_macro(macro, list->next(), env);
}
