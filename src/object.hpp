#ifndef __OBJECT_HPP__
#define __OBJECT_HPP__

#include <functional>
#include <string>
#include <memory>

class Env;

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
    Quoted,
    BackQuoted,
    Comma,
    CommaAtmark,

    // Objects which internally used and user can't create these object.
    FuncPtr,
    PartiallyAppliedFuncPtr,
};

class Object {
public:
    virtual ~Object() {}
    virtual ObjectKind kind() const = 0;
    virtual std::string debug() const = 0;
};

class ListObject : public Object, public std::enable_shared_from_this<ListObject> {
private:
    std::shared_ptr<Object> __value;
    std::shared_ptr<ListObject> __next;
    std::shared_ptr<ListObject> __last;

public:
    ListObject(const std::shared_ptr<Object> value);
    ListObject(const std::shared_ptr<Object> value, std::shared_ptr<ListObject> next);
    std::shared_ptr<Object> value() const;
    std::shared_ptr<ListObject> next() const;
    std::shared_ptr<ListObject> last();
    int size() const;
    void append(const std::shared_ptr<Object> value);
    void append(const std::shared_ptr<ListObject> list);
    ObjectKind kind() const override;
    std::string debug() const override;
};

class TObject : public Object {
public:
    TObject();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class NILObject : public Object {
public:
    NILObject();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class IntegerObject : public Object {
private:
    int __integer;

public:
    IntegerObject(int integer);
    int integer() const;
    ObjectKind kind() const override;
    std::string debug() const override;
};

class NumberObject : public Object {
private:
    double __number;

public:
    NumberObject(double number);
    double number() const;
    ObjectKind kind() const override;
    std::string debug() const override;
};

class StringObject : public Object {
private:
    std::string __string;

public:
    StringObject(const std::string& string);
    std::string& string();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class SymbolObject : public Object {
private:
    std::string __symbol;

public:
    SymbolObject(const std::string& symbol);
    std::string& symbol();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class FunctionObject : public Object {
private:
    std::shared_ptr<Object> __params; // This is either ListObject or NILObject.
                                      // If ListObject, all element is SymbolObject.
    std::shared_ptr<ListObject> __body;

public:
    FunctionObject(const std::shared_ptr<Object> params, const std::shared_ptr<ListObject> body);
    std::shared_ptr<Object> params() const;
    std::shared_ptr<ListObject> body() const;
    ObjectKind kind() const override;
    std::string debug() const override;
};

class PartiallyAppliedFunctionObject : public Object {
private:
    std::shared_ptr<FunctionObject> __func;
    std::shared_ptr<ListObject> __args;

public:
    PartiallyAppliedFunctionObject(
        const std::shared_ptr<FunctionObject> func,
        const std::shared_ptr<ListObject> args
    );
    std::shared_ptr<FunctionObject> func() const;
    std::shared_ptr<ListObject> args() const;
    ObjectKind kind() const override;
    std::string debug() const override;
};

class MacroObject : public Object {
private:
    std::shared_ptr<Object> __params; // This is either ListObject or NILObject.
                                      // If ListObject, all element is SymbolObject.
    std::shared_ptr<ListObject> __body;

public:
    MacroObject(const std::shared_ptr<Object> params, const std::shared_ptr<ListObject> body);
    std::shared_ptr<Object> params() const;
    std::shared_ptr<ListObject> body() const;
    ObjectKind kind() const override;
    std::string debug() const override;
};

class QuotedObject : public Object {
private:
    std::shared_ptr<Object> __object;

public:
    QuotedObject(const std::shared_ptr<Object> object);
    std::shared_ptr<Object> object();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class BackQuotedObject : public Object {
private:
    std::shared_ptr<Object> __object;

public:
    BackQuotedObject(const std::shared_ptr<Object> object);
    std::shared_ptr<Object> object();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class CommaObject : public Object {
private:
    std::shared_ptr<Object> __object;

public:
    CommaObject(const std::shared_ptr<Object> object);
    std::shared_ptr<Object> object();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class CommaAtmarkObject : public Object {
private:
    std::shared_ptr<Object> __object;

public:
    CommaAtmarkObject(const std::shared_ptr<Object> object);
    std::shared_ptr<Object> object();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class FuncPtrObject : public Object {
private:
    std::function<std::shared_ptr<Object>(const std::shared_ptr<ListObject>, Env&)> __func;

public:
    FuncPtrObject(std::function<std::shared_ptr<Object>(const std::shared_ptr<ListObject>, Env&)> func);
    std::function<std::shared_ptr<Object>(const std::shared_ptr<ListObject>, Env&)>& func();
    ObjectKind kind() const override;
    std::string debug() const override;
};

class PartiallyAppliedFuncPtrObject : public Object {
private:
    std::shared_ptr<FuncPtrObject> __func;
    std::shared_ptr<ListObject> __args;

public:
    PartiallyAppliedFuncPtrObject(std::shared_ptr<FuncPtrObject> func, std::shared_ptr<ListObject> args);
    std::shared_ptr<FuncPtrObject> func();
    std::shared_ptr<ListObject> args();
    ObjectKind kind() const override;
    std::string debug() const override;
};

#endif
