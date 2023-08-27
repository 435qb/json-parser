#include <cstddef>
#include <deque>
#include <exception>
#include <iostream>
#include <format>
#include "Lex.hpp"

int main(int argc, char **argv) {
    std::string s;
    while (std::cin >> s) {
        Lexer lexer(s);
        try {
            auto tokens = lexer.dump_tokens();
            for(auto && token : tokens){
                auto value = token.get_value();
                std::string colomn = value.empty() ? "" : ":";
                std::cout << std::format("{}:{}:<{}>{}{}",token.lineno, token.col_offset, token.get_type(), colomn, value) << "\n";
            }
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << "\n";
        }
    }

    return 0;
}
