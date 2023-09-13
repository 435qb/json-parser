#include "Reader.hpp"
#include <cstddef>
#include <deque>
#include <exception>
#include <format>
#include <iostream>
#include <string>

int main() {
    std::string s;
    while (std::getline(std::cin, s)) {
        try {
            auto json = threaded_parse(s);
            std::cout << json.dump() << "\n";
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << "\n";
        }
    }

    return 0;
}
