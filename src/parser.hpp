#ifndef __PARSER_HPP__
#define __PARSER_HPP__

#include "object.hpp"
#include "token.hpp"
#include <list>
#include <memory>

std::list<std::shared_ptr<Object>> parse(const std::list<std::shared_ptr<Token>>& tokens);

#endif
