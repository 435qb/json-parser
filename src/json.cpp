#include "json.hpp"
#include <charconv>
#include <iomanip>
#include <sstream>

template <> Json::Json(ArrayType /**/) : data(std::in_place_type<arraytype>) {}
template <>
Json::Json(ObjectType /**/) : data(std::in_place_type<objecttype>) {}

const Json &Json::operator[](std::string index) const {
    if (!std::holds_alternative<objecttype>(data)) {
        throw std::logic_error("only object can use string index");
    }
    const auto &map = std::get<objecttype>(data);
    return map.at(index);
}
// NOLINTBEGIN(*-no-recursion)
std::string array_print(const Json::arraytype &array, int size,
                        size_t level = 0) {
    if (array.empty()) {
        return "[]";
    }
    std::stringstream s;
    s << "[";
    std::string newline = size == 0 ? "" : "\n";
    std::string levels = newline + std::string(size * (level + 1), ' ');

    bool first = true;
    for (auto &&[_, v] : array) {
        if (!first) {
            s << ",";
        }
        first = false;
        s << levels << v.dump(size, level + 1);
    }
    s << newline << std::string(size * level, ' ') << "]";
    return s.str();
}

std::string object_print(const Json::objecttype &map, int size,
                         size_t level = 0) {
    if (map.empty()) {
        return "{}";
    }
    std::stringstream s;
    s << "{";
    std::string newline = size == 0 ? "" : "\n";
    std::string space = size == 0 ? "" : " ";
    std::string levels = newline + std::string(size * (level + 1), ' ');
    bool first = true;
    for (auto &&[k, v] : map) {
        if (!first) {
            s << ",";
        }
        first = false;
        s << levels << std::quoted(k) << ":" << space << v.dump(size, level + 1);
    }
    s << newline << std::string(size * level, ' ') << "}";
    return s.str();
}

std::string Json::dump(int size, size_t level) const {
    if (std::holds_alternative<Null>(data)) {
        return "null";
    }
    if (std::holds_alternative<bool>(data)) {
        if (std::get<bool>(data)) {
            return "true";
        }
        return "false";
    }
    if (std::holds_alternative<double>(data)) {
        std::string s(255, '\0');
        auto number = std::get<double>(data);
        auto p = std::to_chars(s.begin().base(), s.end().base(), number);
        if (p.ec == std::errc::value_too_large) [[unlikely]] {
            return std::to_string(number);
        }
        s.resize(p.ptr - s.begin().base());
        s.shrink_to_fit();
        return s;
    }
    if (std::holds_alternative<std::string>(data)) {
        std::string retn = "\"";
        for (auto &&ch : std::get<std::string>(data)) {
            switch (ch) {
            case '\x08':
                retn.append("\\b");
                break;
            case '\x09':
                retn.append("\\t");
                break;
            case '\x0A':
                retn.append("\\n");
                break;
            case '\x0C':
                retn.append("\\f");
                break;
            case '\x0D':
                retn.append("\\r");
                break;
            case '\0' ... '\x07':
            case '\x0B':
            case '\x0E' ... '\x19': {
                retn.append("\\u00");
                constexpr static const char alphabeta[] = "0123456789ABCDEF";
                retn.push_back(alphabeta[ch >> 4]);  // NOLINT
                retn.push_back(alphabeta[ch & 0xF]); // NOLINT
                break;
            }
            case '\x22': // "
            case '\x5C': /* \  */
                retn.push_back('\\');
            default:
                retn.push_back(ch);
                break;
            }
        }
        return retn + "\"";
    }
    if (std::holds_alternative<arraytype>(data)) {
        auto map = std::get<arraytype>(data);
        return array_print(map, size, level);
    }
    if (std::holds_alternative<objecttype>(data)) {
        auto map = std::get<objecttype>(data);
        return object_print(map, size, level);
    }
    return "";
}
// NOLINTEND(*-no-recursion)
Json &Json::operator[](std::string index) {
    if (!std::holds_alternative<objecttype>(data)) {
        throw std::logic_error("only object can use string index");
    }
    auto &map = std::get<objecttype>(data);
    return map[index];
}
const Json &Json::operator[](size_t index) const {
    if (!std::holds_alternative<arraytype>(data)) {
        throw std::logic_error("only array can use integer index");
    }
    const auto &map = std::get<arraytype>(data);
    if (index >= map.size()) {
        throw std::logic_error("index out of range");
    }
    return map.at(index);
}
Json &Json::operator[](size_t index) {
    if (!std::holds_alternative<arraytype>(data)) {
        throw std::logic_error("only array can use integer index");
    }
    auto &map = std::get<arraytype>(data);
    if (index > map.size()) {
        throw std::logic_error("index out of range");
    }
    return map[index];
}
void Json::append(Json json) {
    if (!std::holds_alternative<arraytype>(data)) {
        throw std::logic_error("only array can append");
    }
    auto &map = std::get<arraytype>(data);
    map[map.size()] = json;
}
bool Json::contains(size_t index) const {
    if (!std::holds_alternative<arraytype>(data)) {
        throw std::logic_error("only array can use integer index");
    }
    const auto &map = std::get<arraytype>(data);
    return index >= map.size();
}
bool Json::contains(std::string index) const {
    if (!std::holds_alternative<objecttype>(data)) {
        throw std::logic_error("only object can use string index");
    }
    const auto &map = std::get<objecttype>(data);
    return map.contains(index);
}
