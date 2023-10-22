#include "object.hpp"
#include <functional>
#include <memory>
#include <string>

ListObject::ListObject(const std::shared_ptr<Object> value)
    :__value(value), __next(nullptr), __last(nullptr) {}

ListObject::ListObject(const std::shared_ptr<Object> value, std::shared_ptr<ListObject> next)
    : __value(value), __next(next), __last(next->last()) {}

std::shared_ptr<Object> ListObject::value() const {
    return __value;
}

std::shared_ptr<ListObject> ListObject::next() const {
    return __next;
}

std::shared_ptr<ListObject> ListObject::last() {
    // NOTE: Initialize __last here because if I initialize this at constructor with __last(shared_from_this),
    //       this will throw bad_weak_ptr, so instead initialize __last here.
    if (__last == nullptr) {
        if (__next == nullptr) {
            __last = shared_from_this();
        } else {
            __last = __next->last();
        }
    }
    return __last;
}

int ListObject::size() const {
    int len = 0;
    auto it = shared_from_this();
    while (it != nullptr) {
        len += 1;
        it = it->next();
    }
    return len;
}

void ListObject::append(const std::shared_ptr<Object> value) {
    auto new_last = std::make_shared<ListObject>(value);
    last()->__next = new_last;
    __last = new_last;
}

void ListObject::append(const std::shared_ptr<ListObject> list) {
    last()->__next = list;
    __last = list->__last;
}

ObjectKind ListObject::kind() const {
    return ObjectKind::List;
}

std::string ListObject::debug() const {
    std::string s = "(";
    auto it = shared_from_this();
    while (it != nullptr) {
        s += it->value()->debug() + " ";
        it = it->next();
    }
    return s.substr(0, s.size() - 1) + ")";
}

TObject::TObject() {}

ObjectKind TObject::kind() const {
    return ObjectKind::T;
}

std::string TObject::debug() const {
    return "T";
}

NILObject::NILObject() {}

ObjectKind NILObject::kind() const {
    return ObjectKind::NIL;
}

std::string NILObject::debug() const {
    return "NIL";
}

IntegerObject::IntegerObject(int integer) : __integer(integer) {}

int IntegerObject::integer() const {
    return __integer;
}

ObjectKind IntegerObject::kind() const {
    return ObjectKind::Integer;
}

std::string IntegerObject::debug() const {
    return std::to_string(__integer);
}

NumberObject::NumberObject(double number) : __number(number) {}

double NumberObject::number() const {
    return __number;
}

ObjectKind NumberObject::kind() const {
    return ObjectKind::Number;
}

std::string NumberObject::debug() const {
    return std::to_string(__number);
}

StringObject::StringObject(const std::string& string) : __string(string) {}

std::string& StringObject::string() {
    return __string;
}

ObjectKind StringObject::kind() const {
    return ObjectKind::String;
}

std::string StringObject::debug() const {
    return __string;
}

SymbolObject::SymbolObject(const std::string& symbol) : __symbol(symbol) {}

std::string& SymbolObject::symbol() {
    return __symbol;
}

ObjectKind SymbolObject::kind() const {
    return ObjectKind::Symbol;
}

std::string SymbolObject::debug() const {
    return __symbol;
}

FunctionObject::FunctionObject(
    const std::shared_ptr<Object> params,
    const std::shared_ptr<ListObject> body
) : __params(params), __body(body) {}

std::shared_ptr<Object> FunctionObject::params() const {
    return __params;
}

std::shared_ptr<ListObject> FunctionObject::body() const {
    return __body;
}

ObjectKind FunctionObject::kind() const {
    return ObjectKind::Function;
}

std::string FunctionObject::debug() const {
    return "<# FUNCTION" + __params->debug() + " " + __body->debug() + " #>";
}

PartiallyAppliedFunctionObject::PartiallyAppliedFunctionObject(
    const std::shared_ptr<FunctionObject> func,
    const std::shared_ptr<ListObject> args
) : __func(func), __args(args) {}

std::shared_ptr<FunctionObject> PartiallyAppliedFunctionObject::func() const {
    return __func;
}

std::shared_ptr<ListObject> PartiallyAppliedFunctionObject::args() const {
    return __args;
}

ObjectKind PartiallyAppliedFunctionObject::kind() const {
    return ObjectKind::PartiallyAppliedFunction;
}

std::string PartiallyAppliedFunctionObject::debug() const {
    return __func->debug() + " " + __args->debug();
}

MacroObject::MacroObject(
    const std::shared_ptr<Object> params,
    const std::shared_ptr<ListObject> body
) : __params(params), __body(body) {}

std::shared_ptr<Object> MacroObject::params() const {
    return __params;
}

std::shared_ptr<ListObject> MacroObject::body() const {
    return __body;
}

ObjectKind MacroObject::kind() const {
    return ObjectKind::Macro;
}

std::string MacroObject::debug() const {
    return "<# MACRO " + __params->debug() + " " + __body->debug() + " #>";
}

QuotedObject::QuotedObject(const std::shared_ptr<Object> object) : __object(object) {}

std::shared_ptr<Object> QuotedObject::object() {
    return __object;
}

ObjectKind QuotedObject::kind() const {
    return ObjectKind::Quoted;
}

std::string QuotedObject::debug() const {
    return "'" + __object->debug();
}

BackQuotedObject::BackQuotedObject(const std::shared_ptr<Object> object) : __object(object) {}

std::shared_ptr<Object> BackQuotedObject::object() {
    return __object;
}

ObjectKind BackQuotedObject::kind() const {
    return ObjectKind::BackQuoted;
}

std::string BackQuotedObject::debug() const {
    return "`" + __object->debug();
}

CommaObject::CommaObject(const std::shared_ptr<Object> object) : __object(object) {}

std::shared_ptr<Object> CommaObject::object() {
    return __object;
}

ObjectKind CommaObject::kind() const {
    return ObjectKind::Comma;
}

std::string CommaObject::debug() const {
    return "," + __object->debug();
}

CommaAtmarkObject::CommaAtmarkObject(const std::shared_ptr<Object> object) : __object(object) {}

std::shared_ptr<Object> CommaAtmarkObject::object() {
    return __object;
}

ObjectKind CommaAtmarkObject::kind() const {
    return ObjectKind::CommaAtmark;
}

std::string CommaAtmarkObject::debug() const {
    return ",@" + __object->debug();
}

FuncPtrObject::FuncPtrObject(
    std::function<std::shared_ptr<Object>(const std::shared_ptr<ListObject>, Env&)> func
) : __func(func) {}

std::function<std::shared_ptr<Object>(const std::shared_ptr<ListObject>, Env&)>& FuncPtrObject::func() {
    return __func;
}

ObjectKind FuncPtrObject::kind() const {
    return ObjectKind::FuncPtr;
}

std::string FuncPtrObject::debug() const {
    return "function pointer";
}

PartiallyAppliedFuncPtrObject::PartiallyAppliedFuncPtrObject(
    std::shared_ptr<FuncPtrObject> func,
    std::shared_ptr<ListObject> args
) : __func(func), __args(args) {}

std::shared_ptr<FuncPtrObject> PartiallyAppliedFuncPtrObject::func() {
    return __func;
}

std::shared_ptr<ListObject> PartiallyAppliedFuncPtrObject::args() {
    return __args;
}

ObjectKind PartiallyAppliedFuncPtrObject::kind() const {
    return ObjectKind::PartiallyAppliedFuncPtr;
}

std::string PartiallyAppliedFuncPtrObject::debug() const {
    return "partially applied function pointer";
}
