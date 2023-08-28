#include "json.hpp"
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
    std::string levels = "\n" + std::string(size * (level + 1), ' ');

    bool first = true;
    for (auto &&[_, v] : array) {
        if (!first) {
            s << ",";
        }
        first = false;
        s << levels << v.dump(size, level + 1);
    }
    s << "\n" << std::string(size * level, ' ') << "]";
    return s.str();
}

std::string object_print(const Json::objecttype &map, int size,
                         size_t level = 0) {
    if (map.empty()) {
        return "{}";
    }
    std::stringstream s;
    s << "{";
    std::string levels = "\n" + std::string(size * (level + 1), ' ');
    bool first = true;
    for (auto &&[k, v] : map) {
        if (!first) {
            s << ",";
        }
        first = false;
        s << levels << "\"" << k << "\": " << v.dump(size, level + 1);
    }
    s << "\n" << std::string(size * level, ' ') << "}";
    return s.str();
}
std::string Json::dump(int size, size_t level) const {
    if (std::holds_alternative<Null>(data)) {
        return "null";
    }
    if (std::holds_alternative<bool>(data)) {
        return std::to_string(std::get<bool>(data));
    }
    if (std::holds_alternative<double>(data)) {
        return std::to_string(std::get<double>(data));
    }
    if (std::holds_alternative<std::string>(data)) {
        return std::get<std::string>(data);
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
const Json &Json::operator[](int index) const {
    if (!std::holds_alternative<arraytype>(data)) {
        throw std::logic_error("only array can use integer index");
    }
    const auto &map = std::get<arraytype>(data);
    if (index >= map.size()) {
        throw std::logic_error("index out of range");
    }
    return map.at(index);
}
Json &Json::operator[](int index) {
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
bool Json::contains(int index) const {
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
