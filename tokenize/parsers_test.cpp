#include "acutest.h"
#include "parsers.hpp"
#include "readers.hpp"

using namespace tokenize::parsers;
using tokenize::readers::make_string_reader;

// digit
void digit_success_1_test() {
    auto reader = make_string_reader("1");
    std::string s;
    TEST_ASSERT(digit()(reader, s));
}

// multi
void multi_success_test() {
    auto reader = make_string_reader("hello");
    std::string s;
    const auto p = reader->get_position();
    TEST_ASSERT(multi("hello")(reader, s));
    TEST_ASSERT(s == "hello");
    TEST_ASSERT(reader->get_position() != p);
}

void multi_failed_0_test() {
    auto reader = make_string_reader("hello");
    std::string s;
    const auto p = reader->get_position();
    TEST_ASSERT(!multi("world")(reader, s));
    TEST_ASSERT(reader->get_position() == p);
}

void multi_failed_1_test() {
    auto reader = make_string_reader("hello");
    std::string s;
    const auto p = reader->get_position();
    TEST_ASSERT(!multi("hola")(reader, s));
    TEST_ASSERT(reader->get_position() != p);
}

// multi_list
void multi_list_success_0_test() {
    auto parser = multi_list({"hello", "hola"});
    {
        auto reader = make_string_reader("hola");
        std::string s;
        TEST_ASSERT(parser(reader, s) && s == "hola");
    }
    {
        auto reader = make_string_reader("hello");
        std::string s;
        TEST_ASSERT(parser(reader, s) && s == "hello");
    }
}

void multi_list_failed_0_test() {
    auto parser = multi_list({"hello", "hola"});
    {
        auto reader = make_string_reader("bye");
        std::string s;
        TEST_ASSERT(!parser(reader, s));
    }
    {
        auto reader = make_string_reader("hi world");
        std::string s;
        TEST_ASSERT(!parser(reader, s));
    }
}

// commnet
void commnet_success_line_test() {
    auto reader = make_string_reader("//ab\n");
    {
        std::string s;
        TEST_ASSERT(comment(reader, s) && s == "//ab\n");
    }
}

void commnet_success_block_test() {
    auto reader = make_string_reader("/*abc*/");
    {
        std::string s;
        TEST_ASSERT(comment(reader, s) && s == "/*abc*/");
    }
}

// variable
void variable_success_alpha_test() {
    auto reader = make_string_reader("hello");
    {
        std::string s;
        TEST_ASSERT(variable(reader, s) && s == "hello");
    }
}

void variable_success_alnum_test() {
    auto reader = make_string_reader("abc123");
    {
        std::string s;
        TEST_ASSERT(variable(reader, s) && s == "abc123");
    }
}

void variable_failed_number_test() {
    auto reader = make_string_reader("1");
    {
        std::string s;
        TEST_ASSERT(!variable(reader, s));
    }
}

// character
void character_success_alpha_test() {
    auto reader = make_string_reader("'a'");
    {
        std::string s;
        TEST_ASSERT(character(reader, s) && s == "'a'");
    }
}

void character_success_newline_test() {
    auto reader = make_string_reader("'\n'");
    {
        std::string s;
        TEST_ASSERT(character(reader, s) && s == "'\n'");
    }
}

void character_failed_empty_test() {
    auto reader = make_string_reader("''");
    {
        std::string s;
        TEST_ASSERT(!character(reader, s));
    }
}

void character_failed_over_test() {
    auto reader = make_string_reader("'01'");
    {
        std::string s;
        TEST_ASSERT(!character(reader, s));
    }
}

TEST_LIST = {
    // digit
    {"digit_success_1_test",digit_success_1_test},
    // multi
    {"multi_success_test", multi_success_test},
    {"multi_faield_0_test", multi_failed_0_test},
    {"multi_faield_1_test", multi_failed_1_test},
    // multi list
    {"multi_list_success_0_test", multi_list_success_0_test},
    {"multi_list_failed_0_test", multi_list_failed_0_test},
    // comment
    {"commnet_success_line_test", commnet_success_line_test},
    {"commnet_success_block_test", commnet_success_block_test},
    // variable
    {"variable_success_alpha_test", variable_success_alpha_test},
    {"variable_success_alnum_test", variable_success_alnum_test},
    {"variable_failed_number_test", variable_failed_number_test},
    // charactor
    {"character_success_alpha_test", character_success_alpha_test},
    {"character_success_newline_test", character_success_newline_test},
    {"character_failed_empty_test", character_failed_empty_test},
    {"character_failed_over_test", character_failed_over_test},
    // end
    {nullptr, nullptr}};