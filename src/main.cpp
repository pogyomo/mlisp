#include "env.hpp"
#include "object.hpp"
#include "token.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "eval.hpp"
#include <exception>
#include <iostream>
#include <istream>

std::istream& prompt(std::istream& is, const std::string& msg, std::string& input) {
    std::cout << msg << " " << std::flush;
    return std::getline(is, input);
}

int main() {
    Env env = default_env();
    std::string input;
    try {
        while (prompt(std::cin, ">", input)) {
            auto tokens = lex(input);
            auto objs = parse(tokens);
            std::cout << eval(*objs.begin(), env)->debug() << std::endl;
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
