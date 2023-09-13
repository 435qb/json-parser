#ifndef READER_HPP
#define READER_HPP
#include "json.hpp"
#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>
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
    std::variant<std::string, double> value{};

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

struct Parser {
    mutable std::mutex m;
    mutable std::condition_variable cv;
    std::deque<Token> tokens;
    bool stop = false;
    std::deque<Token>::const_iterator curr(int k = 0) const{
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [this, k](){return stop || tokens.size() > k;});
        if(stop){
            throw 0;
        }
        return tokens.cbegin() + k;
    }
    Parser() = default;

    Json parse();
    Json parse_value();
    Json parse_object();
    Json parse_array();
    [[noreturn]] void error(const char *messgae) const;
    void inline next(int step = 1){
        while(step--){
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [this](){return stop || !tokens.empty();});
            if(stop){
                throw 0;
            }
            tokens.pop_front();
        }
    }
    void push(Token token){
        std::unique_lock<std::mutex> lk(m);
        tokens.emplace_back(std::move(token));
        cv.notify_one();
    }
};

Json threaded_parse(std::string_view data);
#endif // READER_HPP