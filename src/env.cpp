#include "env.hpp"
#include "object.hpp"
#include "eval.hpp"
#include <memory>
#include <stdexcept>

EnvException::EnvException(const std::string& msg) : std::runtime_error(msg) {}

Env::Env() {}

Env::Env(std::shared_ptr<Env> outer) : outer(outer) {}

std::shared_ptr<Object> Env::get_obj(const std::string& sym) {
    try {
        return symtable.at(sym);
    } catch (const std::out_of_range& _) {
        if (outer == nullptr) {
            return std::make_shared<NILObject>();
        } else {
            throw EnvException("no such symbol exist: " + sym);
        }
    }
}

void Env::set_obj(const std::string& sym, const std::shared_ptr<Object> obj) {
    symtable[sym] = obj;
}

Env default_env() {
    Env env;
    env.set_obj("=", std::make_shared<FuncPtrObject>(fn_eq_num));
    env.set_obj("/=", std::make_shared<FuncPtrObject>(fn_ne_num));
    env.set_obj("<", std::make_shared<FuncPtrObject>(fn_lt_num));
    env.set_obj(">", std::make_shared<FuncPtrObject>(fn_gt_num));
    env.set_obj("<=", std::make_shared<FuncPtrObject>(fn_le_num));
    env.set_obj(">=", std::make_shared<FuncPtrObject>(fn_ge_num));
    env.set_obj("+", std::make_shared<FuncPtrObject>(fn_add_num));
    env.set_obj("-", std::make_shared<FuncPtrObject>(fn_sub_num));
    env.set_obj("*", std::make_shared<FuncPtrObject>(fn_mul_num));
    env.set_obj("/", std::make_shared<FuncPtrObject>(fn_div_num));
    env.set_obj("list", std::make_shared<FuncPtrObject>(fn_list));
    env.set_obj("labmda", std::make_shared<FuncPtrObject>(fn_lambda));
    env.set_obj("macro", std::make_shared<FuncPtrObject>(fn_macro));
    env.set_obj("macroexpand", std::make_shared<FuncPtrObject>(fn_macroexpand));
    return env;
}
