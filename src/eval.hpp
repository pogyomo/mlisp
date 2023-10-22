#ifndef __EVAL_HPP__
#define __EVAL_HPP__

#include "object.hpp"
#include "env.hpp"
#include <memory>
#include <stdexcept>

class EvalException : public std::runtime_error {
public:
    EvalException(const std::string& msg);
};

std::vector<std::shared_ptr<Object>> take_args(
    const std::string& name,
    std::shared_ptr<ListObject> args,
    int num,
    bool restrict
);
std::vector<std::shared_ptr<Object>> eval_args(
    const std::string& name,
    std::shared_ptr<ListObject> args,
    Env& env,
    int num,
    bool restrict
);
std::shared_ptr<Object> eval(std::shared_ptr<Object> expr, Env& env);
std::shared_ptr<Object> eval_list(std::shared_ptr<ListObject> list, Env& env);
std::shared_ptr<Object> apply_func(
    std::shared_ptr<FunctionObject> func,
    std::shared_ptr<ListObject> args,
    Env& env
);
std::shared_ptr<Object> apply_func_ptr(
    std::shared_ptr<FuncPtrObject> func,
    std::shared_ptr<ListObject> args,
    Env& env
);
std::shared_ptr<Object> apply_macro(
    std::shared_ptr<MacroObject> macro,
    std::shared_ptr<ListObject> args,
    Env& env
);
std::shared_ptr<Object> expand_macro(
    std::shared_ptr<MacroObject> macro,
    std::shared_ptr<ListObject> args,
    Env& env
);
std::shared_ptr<Object> fn_eq_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_ne_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_lt_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_gt_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_le_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_ge_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_add_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_sub_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_mul_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_div_num(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_list(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_lambda(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_macro(std::shared_ptr<ListObject> args, Env& env);
std::shared_ptr<Object> fn_macroexpand(std::shared_ptr<ListObject> args, Env& env);

#endif
