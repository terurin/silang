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
#include <unordered_map>
#include <vector>

namespace tokenize::parsers {

using readers::reader_ptr, readers::position;

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
    const std::unordered_map<std::string, bool> table; // true -> tail, false -> continue, null -> mismatch

public:
    multi_list(const std::vector<std::string> &_keywords);
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

class bracket {
    const parser_t<std::string> begin;
    const parser_t<std::string> body;
    const parser_t<std::string> end;

public:
    bracket(const parser_t<std::string> _begin, const parser_t<std::string> _body, const parser_t<std::string> _end)
        : begin(_begin), body(_body), end(_end) {}
    bool operator()(reader_ptr &, std::string &) const;
};

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
const inline auto escaped_char = satify([](char c) { return c != '\\'; }) + one('\\') * any;
// 空白
const inline auto newline = list({'\n', '\r'});
const inline auto space = list({' ', '\t', '\n', '\r'});
const inline auto spaces = many1(space);

// 特殊
bool eof(reader_ptr &, std::string &);
bool nop(reader_ptr &, std::string &);

// atoms
const inline auto integer =
    option(sign) * (attempt(multi("0b") * many1(digit_base(2)) + multi("0q") * many1(digit_base(4)) +
                            multi("0o") * many1(digit_base(8)) + multi("0d") * many1(digit_base(10)) +
                            multi("0x") * many1(digit_base(16))) +
                    many1(digit_base(10)));

bool real(reader_ptr &, std::string &);
const inline auto text = attempt(bracket(multi("\"\"\""), escaped_char, attempt(multi("\"\"\"")))) +
                         bracket(one('"'), escaped_char, one('"'));
// bool comment(reader_ptr &, std::string &);
const inline auto comment =
    attempt(bracket(multi("//"), any, newline + eof)) + bracket(multi("/*"), any, attempt(multi("*/")));
const inline auto variable = (alpha + one('_')) * many0(alnum + one('_'));
const inline auto character = one('\'') * escaped_char * one('\'');
} // namespace tokenize::parsers
