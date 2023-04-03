#include "acutest.h"
#include "parsers.hpp"
#include "readers.hpp"

using namespace tokenize::parsers;
using tokenize::readers::make_string_reader;

// satify
void satify_success_test() {
    auto reader = make_string_reader("demo", "1");
    char c = 0;
    const auto p = reader->get_position();
    TEST_ASSERT(satify([](char c) { return c == '1'; })(reader, c));
    TEST_ASSERT(c == '1');
    TEST_ASSERT(reader->get_position() != p);
}

void satify_failed_test() {
    auto reader = make_string_reader("demo", "1");
    char c = 0;
    const auto p = reader->get_position();
    TEST_ASSERT(!satify([](char t) { return t == '0'; })(reader, c));
    TEST_ASSERT(c == 0);
    TEST_ASSERT(reader->get_position() == p);
}

void satify_eof_test() {
    auto reader = make_string_reader("demo", "");
    char c;
    TEST_ASSERT(!satify([](char c) { return true; })(reader, c));
}

// multi
void multi_success_test() {
    auto reader = make_string_reader("demo", "hello");
    std::string s;
    const auto p = reader->get_position();
    TEST_ASSERT(multi("hello")(reader, s));
    TEST_ASSERT(s == "hello");
    TEST_ASSERT(reader->get_position() != p);
}

void multi_failed_0_test() {
    auto reader = make_string_reader("demo", "hello");
    std::string s;
    const auto p = reader->get_position();
    TEST_ASSERT(!multi("world")(reader, s));
    TEST_ASSERT(reader->get_position() == p);
}

void multi_failed_1_test() {
    auto reader = make_string_reader("demo", "hello");
    std::string s;
    const auto p = reader->get_position();
    TEST_ASSERT(!multi("hola")(reader, s));
    TEST_ASSERT(reader->get_position() != p);
}

// multi_list
void multi_list_success_0_test() {
    auto parser = multi_list({"hello", "hola"});
    {
        auto reader = make_string_reader("demo", "hola");
        std::string s;
        TEST_ASSERT(parser(reader, s) && s == "hola");
    }
    {
        auto reader = make_string_reader("demo", "hello");
        std::string s;
        TEST_ASSERT(parser(reader, s) && s == "hello");
    }
}

void multi_list_failed_0_test() {
    auto parser = multi_list({"hello", "hola"});
    {
        auto reader = make_string_reader("demo", "bye");
        std::string s;
        TEST_ASSERT(!parser(reader, s));
    }
    {
        auto reader = make_string_reader("demo", "hi world");
        std::string s;
        TEST_ASSERT(!parser(reader, s));
    }
}

TEST_LIST = {
    // safity
    {"satify_success_test", satify_success_test},
    {"satify_failed_test", satify_failed_test},
    {"satify_eof_test", satify_eof_test},
    // multi
    {"multi_success_test", multi_success_test},
    {"multi_faield_0_test", multi_failed_0_test},
    {"multi_faield_1_test", multi_failed_1_test},
    // multi list
    {"multi_list_success_0_test", multi_list_success_0_test},
    {"multi_list_failed_0_test", multi_list_failed_0_test},
    // end
    {nullptr, nullptr}};