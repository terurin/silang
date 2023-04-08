#include "acutest.h"
#include "readers.hpp"
using namespace tokenize::readers;

void position_test() {
    position p;
    TEST_ASSERT(p.offset == 0 && p.line == 0 && p.number == 0);

    // non newline
    p.next('a');
    TEST_ASSERT(p.offset == 1 && p.line == 0 && p.number == 1);

    // newline \n
    p.next('\n');
    TEST_ASSERT(p.offset == 2 && p.line == 1 && p.number == 0);

    // newline \r
    p.next('\r');
    TEST_ASSERT(p.offset == 3 && p.line == 2 && p.number == 0);
}

void string_reader_test() {
    reader_ptr r = make_string_reader( "a");

    // default position
    const auto p0 = r->get_position();

    // peek
    auto peek = r->peek();
    TEST_ASSERT(peek && *peek == 'a');
    TEST_ASSERT(r->get_position() == p0);

    // next
    auto next = r->next();
    TEST_ASSERT(next && *next == 'a');
    TEST_ASSERT(r->get_position() != p0);

    // next empty
    auto empty = r->next();
    TEST_ASSERT(!empty);

    // reset position
    r->set_position(p0);
    TEST_ASSERT(r->get_position() == p0);
    auto reset_next = r->next();
    TEST_ASSERT(reset_next && *reset_next == 'a');
};

TEST_LIST = {{"position_test", position_test}, {"string_reader_test", string_reader_test}, {nullptr, nullptr}};