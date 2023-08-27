#include <gtest/gtest.h>

#include "Lex.hpp"

void check_token(const Token &token, Token::Type type,
                 const std::string &value = "") {
    EXPECT_EQ(token.type, type);
    EXPECT_EQ(std::get<std::string>(token.value), value);
}
void check_token(const Token &token, Token::Type type, double value) {
    EXPECT_EQ(token.type, type);
    EXPECT_DOUBLE_EQ(std::get<double>(token.value), value);
}
// NOLINTBEGIN
TEST(TokenTest, unicode) {
    Lexer lexer(R"("\uD83E\uDD70")");
    check_token(lexer.get_next_token(), Token::Type::STRING, "🥰");
    check_token(lexer.get_next_token(), Token::Type::EOF_);
}
TEST(TokenTest, literals) {
    Lexer lexer(R"(truefalsenull)");
    check_token(lexer.get_next_token(), Token::Type::TRUE);
    check_token(lexer.get_next_token(), Token::Type::FALSE);
    check_token(lexer.get_next_token(), Token::Type::NULL_);
    check_token(lexer.get_next_token(), Token::Type::EOF_);
}
TEST(TokenTest, structural_characters) {
    Lexer lexer(R"([]{}:,)");
    check_token(lexer.get_next_token(), Token::Type::BEGIN_ARRAY);
    check_token(lexer.get_next_token(), Token::Type::END_ARRAY);
    check_token(lexer.get_next_token(), Token::Type::BEGIN_OBJECT);
    check_token(lexer.get_next_token(), Token::Type::END_OBJECT);
    check_token(lexer.get_next_token(), Token::Type::NAME_SEPARATOR);
    check_token(lexer.get_next_token(), Token::Type::VALUE_SEPARATOR);
    check_token(lexer.get_next_token(), Token::Type::EOF_);
}

TEST(TokenTest, strings) {
    Lexer lexer(R"("\ndead\\\/")");
    check_token(lexer.get_next_token(), Token::Type::STRING, "\ndead\\/");
    check_token(lexer.get_next_token(), Token::Type::EOF_);
}
TEST(TokenTest, numbers) {
    Lexer lexer(R"(010.23,-10.23e-1)");
    check_token(lexer.get_next_token(), Token::Type::NUMBER, 0);
    check_token(lexer.get_next_token(), Token::Type::NUMBER, 10.23);
    check_token(lexer.get_next_token(), Token::Type::VALUE_SEPARATOR);
    check_token(lexer.get_next_token(), Token::Type::NUMBER, -10.23e-1);
    check_token(lexer.get_next_token(), Token::Type::EOF_);
}
TEST(TokenTest, exceptions) {
    {
        Lexer lexer(R"(")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"(ftrue)");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\u")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\u7abg")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\uD83Ea")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\uD83E\u01")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\uD83E\uABCg")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\uD83E\uD83E")");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"("\\)");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"(true-)");
        lexer.get_next_token();
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"(:-))");
        lexer.get_next_token();
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"(1.A)");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"(1.1e)");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
    {
        Lexer lexer(R"(1e[])");
        EXPECT_ANY_THROW({ lexer.get_next_token(); });
    }
}
// NOLINTEND

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}