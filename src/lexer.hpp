#ifndef __LEXER_HPP__
#define __LEXER_HPP__

#include "token.hpp"
#include <list>
#include <memory>

std::list<std::shared_ptr<Token>> lex(const std::string& input);

#endif
