#include "Reader.hpp"
#include <cstddef>
#include <deque>
#include <exception>
#include <format>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
    std::string s;
    while (std::getline(std::cin, s)) {
        Lexer lexer(s);
        try {
            auto tokens = lexer.dump_tokens();
            // for(auto && token : tokens){
            //     auto value = token.get_value();
            //     std::string colomn = value.empty() ? "" : ":";
            //     std::cout << std::format("{}:{}:<{}>{}{}",token.lineno, token.col_offset, token.get_type(), colomn, value) << "\n";
            // }
            Parser parser(tokens);
            auto json = parser.parse();
            std::cout << json.dump() << "\n";
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << "\n";
        }
    }

    return 0;
}
