#ifndef JSON_HPP
#define JSON_HPP

#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

struct Null {
    friend bool operator==(const Null & /*unused*/, const Null & /*unused*/) {
        return true;
    }
};
struct ArrayType {};
struct ObjectType {};
struct Json {
    using arraytype = std::map<size_t, Json>;
    using objecttype = std::unordered_map<std::string, Json>;
    std::variant<Null, bool, double, std::string, arraytype, objecttype> data;

    template <class T> explicit Json(T b) : data(b) {}
    Json() = default;
    void append(Json json);
    Json &operator[](int index);
    const Json &operator[](int index) const;

    Json &operator[](std::string index);
    const Json &operator[](std::string index) const;

    bool contains(int index) const;
    bool contains(std::string index) const;
    std::string dump(int size = 4) const { return dump(size, 0); }
    std::string dump(int size, size_t level) const;

    friend bool operator==(const Json &lhs, const Json &rhs) {
        return lhs.data == rhs.data;
    }
};
template <> Json::Json(ArrayType);
template <> Json::Json(ObjectType);
#endif // JSON_HPP