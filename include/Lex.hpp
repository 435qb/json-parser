#ifndef LEX_HPP
#define LEX_HPP
#include <string>
#include <variant>
#include <vector>

struct Token {
    int lineno, col_offset;
    enum class Type {
        EOF_,
        BEGIN_ARRAY,     // [ left square bracket
        BEGIN_OBJECT,    // { left curly bracket
        END_ARRAY,       // ] right square bracket
        END_OBJECT,      // } right curly bracket
        NAME_SEPARATOR,  // : colon
        VALUE_SEPARATOR, // , comma
        FALSE,           // , false
        TRUE,            // , true
        NULL_,           // , null
        NUMBER,
        STRING
    } type;
    std::variant<std::string, double> value;

    std::string get_type() const;
    std::string get_value() const;
};

struct Lexer {
  private:
    int lineno = 1;
    int colnom = 1;
    std::string_view data_;
    std::string_view::iterator curr_pos;

  public:
    explicit Lexer(std::string_view data)
        : data_(data), curr_pos(data_.begin()) {}

    Token get_next_token();
    std::vector<Token> dump_tokens();

  private:
    Token generate_token(Token::Type type, std::string value) const;
    Token generate_token(Token::Type type, double value) const;
    Token generate_token(Token::Type type) const;

    Token get_string();
    Token get_number();
    [[noreturn]] void error(const char *messgae) const;

    void skip_ws();
    void inline next(int step = 1) {
        colnom += step;
        curr_pos += step;
    }

    template <size_t N> bool match(const char (&string)[N]) {
        return data_.substr(curr_pos - data_.begin(), N - 1) == string; // '\0'
    }
};

#endif // LEX_HPP