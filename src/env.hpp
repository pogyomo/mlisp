#ifndef __ENV_HPP__
#define __ENV_HPP__

#include "object.hpp"
#include <memory>
#include <map>
#include <stdexcept>

class Object;

class EnvException : public std::runtime_error {
public:
    EnvException(const std::string& msg);
};

class Env {
private:
    std::map<std::string, std::shared_ptr<Object>> symtable;
    std::shared_ptr<Env> outer;

public:
    Env();
    Env(std::shared_ptr<Env> outer);
    std::shared_ptr<Object> get_obj(const std::string& sym);
    void set_obj(const std::string& sym, const std::shared_ptr<Object> obj);
};

Env default_env();

#endif
