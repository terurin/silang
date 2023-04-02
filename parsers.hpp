#pragma once

#include "readers.hpp"
#include <algorithm>
#include <assert.h>
#include <climits>
#include <concepts>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace parsers {

using readers::reader_ptr, readers::position;
using std::string;

template <class P, class T = std::string>
concept parser = std::predicate<reader_ptr &, T &>;
template <class T = std::string> using parser_t = std::function<bool(reader_ptr &, T &)>;

class satify {
    const std::function<bool(char)> predicate;

public:
    satify(const std::function<bool(char)> &_predicate) : predicate(_predicate) {}
    bool operator()(reader_ptr &, char &) const;
    bool operator()(reader_ptr &, std::string &) const;
};

class multi {
    const std::string keyword;

public:
    multi(std::string_view sv) : keyword(sv) {}
    bool operator()(reader_ptr &, std::string &) const;
};

class multi_list {
    const std::vector<std::string> keywords;                                         // sort by length
    static std::vector<std::string> keywords_sort(const std::vector<std::string> &); // only use initialize

public:
    multi_list(const std::vector<std::string> &_keywords) : keywords(keywords_sort(_keywords)) {}
    bool operator()(reader_ptr &, std::string &) const;
};

class chain {
    const parser_t<std::string> right;
    const parser_t<std::string> left;

public:
    chain(const parser_t<std::string> &_right, const parser_t<std::string> &_left) : right(_right), left(_left) {}
    bool operator()(reader_ptr &, std::string &) const;
};

static inline chain operator*(const parser_t<std::string> &r, const parser_t<std::string> &l) { return chain(r, l); }

class repeat_range {
    const parser_t<std::string> parser;
    const unsigned int min, max;

public:
    repeat_range(const parser_t<std::string> &_parser, unsigned int _min = 0, unsigned int _max = UINT_MAX);
    bool operator()(reader_ptr &, std::string &) const;
};

static inline repeat_range many0(const parser_t<std::string> &parser) { return repeat_range(parser, 0); }
static inline repeat_range many1(const parser_t<std::string> &parser) { return repeat_range(parser, 1); }
static inline repeat_range option(const parser_t<std::string> &parser) { return repeat_range(parser, 0, 1); }

static inline repeat_range repeat(const parser_t<std::string> &parser, unsigned int n) {
    return repeat_range(parser, n, n);
}

class attempt {
    const parser_t<std::string> parser;

public:
    attempt(const parser_t<std::string> &_parser) : parser(_parser) {}
    bool operator()(reader_ptr &, std::string &) const;
};

class sum {
    const parser_t<std::string> right;
    const parser_t<std::string> left;

public:
    sum(const parser_t<std::string> &_right, const parser_t<std::string> &_left) : right(_right), left(_left) {}
    bool operator()(reader_ptr &, std::string &) const;
};

static inline sum operator+(const parser_t<std::string> &r, const parser_t<std::string> &l) { return sum(r, l); }

// token series

satify one(char token);
satify range(char begin, char end);
satify list(std::initializer_list<char> list);
satify list(std::string_view items);

const inline satify any([](char _) { return true; });
const inline satify none([](char _) { return false; });

// 整数関係
satify digit_base(unsigned int base = 10);
const inline satify sign = list({'+', '-'});
const inline satify base_list = list({'b', 'q', 'o', 'd', 'x'});
std::optional<unsigned int> base_number(char c);
const inline satify digit_bin = digit_base(2);
const inline satify digit_quad = digit_base(4);
const inline satify digit_dec = digit_base(10);
const inline satify digit = digit_dec;
const inline satify digit_hex = digit_base(16);
// アルファベット
const inline satify small = range('a', 'z');
const inline satify large = range('A', 'Z');
const inline auto alpha = small + large;
const inline auto alnum = small + large + digit;

// 空白
const inline auto newline = list({'\n', '\r'});
const inline auto space = list({' ', '\t', '\n', '\r'});
const inline auto spaces = many1(space);

// 特殊
bool eof(reader_ptr &, std::string &);
bool nop(reader_ptr &, std::string &);

// atoms
bool integer(reader_ptr &, std::string &);
bool real(reader_ptr &, std::string &);
bool text(reader_ptr &, std::string &);
bool comment(reader_ptr &, std::string &);
bool variable(reader_ptr &, std::string &);
bool operations(reader_ptr &, std::string &);

// tokens
enum class token_id {
    none = 0,
    // literal
    boolean = 0x10,
    integer,
    real,
    text,
    variable,
    // operations
    op_assign = 0x20, // =
    // arith
    op_add= 0x30,        //+
    op_sub,        //-
    op_mul,        //*
    op_div,        ///
    op_mod,        //%
    op_assign_add, // +=
    op_assign_sub, // -=
    op_assign_mul, // *=
    op_assign_div, // /=
    op_assign_mod, // %=
    // compare
    op_eq = 0x40, //==
    op_ne,        //!=
    op_gt,        // >
    op_ge,        //>=
    op_lt,        //<
    op_le,        //<=
    // bit logic && shift
    op_bitnot = 0x50, //~
    op_bitand,        //&
    op_bitor,         //|
    op_bitxor,        //^
    op_rshfit,        //>>
    op_lshfit,        //<<
    op_assign_bitand, //&=
    op_assign_bitor,  //|=
    op_assign_bitxor, //^=
    op_assign_rshfit, //>>=
    op_assign_lshfit, //<<=

    // bits
    op_not = 0x60, //!
    op_and,        //&&
    op_or,         //||
    op_xor,        //^^
    op_assign_and, //&&=
    op_assign_or,  //||=
    op_assign_xor, //^^=
    // member
    op_member = 0x70, //.
    op_arrow,         //->
    op_at,            //@
    // type opration
    op_option = 0x80, //?

    // bracket& separator
    op_bracket_begin = 0xA0, //(
    op_bracket_end,          //)
    op_bracket_empty,        //()
    op_block_begin,          //{
    op_block_end,            //}
    op_index_begin,          //[
    op_index_end,            //]
    op_line,                 //;
    op_comma,                //,

    // bool
    word_bool = 0x100,
    word_true,
    word_false,
    // types
    word_int,
    word_uint,
    word_str,

};

std::ostream &operator<<(std::ostream &, token_id);
struct token {
    token_id id = token_id::none;
    position pos;
    std::string text;
};

std::ostream &operator<<(std::ostream &, const token &);

class tokener {
    const token_id id;
    const parser_t<std::string> parser;

public:
    tokener(const token_id _id, parser_t<std::string> _parser) : id(_id), parser(_parser) {}
    bool operator()(reader_ptr &, token &) const;
};

bool tokenize(reader_ptr &, token &);
bool tokenize_op(reader_ptr &, token &);

bool tokenize_all(reader_ptr& ,std::vector<token>&);

std::ostream& operator <<(std::ostream& ,const std::vector<token>&);

} // namespace parsers

#include "parsers.tpp"
