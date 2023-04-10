#pragma once

#include "readers.hpp"
#include <algorithm>
#include <assert.h>
#include <bitset>
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

using match_t = std::bitset<256>;
class atom {
    match_t match;

public:
    atom(char);
    atom(std::initializer_list<char>);
    atom(std::string_view);
    atom(const match_t &_match) : match(_match) {}
    atom(const atom &) = default;
    const match_t get_match() const { return match; }

    bool operator()(reader_ptr &, char &) const;
    bool operator()(reader_ptr &, std::string &) const;
};

// 生成関係
atom range(unsigned char first, unsigned char last);
static inline atom one(unsigned char c) { return atom(c); }
static inline atom list(std::string_view sv) { return atom(sv); }

inline atom any = range(0, 255);
inline atom none({});
atom operator+(const atom &, const atom &);
atom operator-(const atom &, const atom &);

// 記号
const inline atom sign = atom("+-");
const inline atom escape = atom({'\\'});
const inline atom not_escape = any-escape;
atom digit(unsigned int n = 10);
const inline atom small = range('a', 'z');
const inline atom large = range('A', 'Z');
const inline atom alpha = small + large;
const inline atom alnum = small + large + digit();
// 空白
const inline auto newline = list("\r\n");
const inline auto space = list(" \t\n\r");

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

template <class T> class attempt {
    const parser_t<T> parser;

public:
    attempt(const parser_t<T> &_parser) : parser(_parser) {}
    bool operator()(reader_ptr &reader, T &out) const;
};

template <class T> class sum {
    const parser_t<T> right;
    const parser_t<T> left;

public:
    sum(const parser_t<T> &_right, const parser_t<T> &_left) : right(_right), left(_left) {}
    bool operator()(reader_ptr &, T &) const;
};

static inline auto operator+(const parser_t<std::string> &r, const parser_t<std::string> &l) {
    return sum<std::string>(r, l);
}

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

// 整数関係

static inline auto escaped_digits(unsigned int n = 10) {
    return many1(digit(n)) * many0(attempt<std::string>(many1(one('_')) * many1(digit(n))));
}

// アルファベット

const inline auto escaped_char = not_escape + one('\\') * any;

const inline auto spaces = many1(space);

// 特殊
bool eof(reader_ptr &, std::string &);
bool nop(reader_ptr &, std::string &);

// integer
const inline auto integer =
    option(sign) *
    (attempt<std::string>(multi("0b") * escaped_digits(2)) + attempt<std::string>(multi("0q") * escaped_digits(4)) +
     attempt<std::string>(multi("0o") * escaped_digits(8)) + attempt<std::string>(multi("0d") * escaped_digits(10)) +
     attempt<std::string>(multi("0x") * escaped_digits(16)) + escaped_digits(10));

// real
const inline auto dot = one('.');
const inline auto mantissa_digits(unsigned int n) { return escaped_digits(n) * dot * escaped_digits(n); }
const inline auto mantissa =
    option(sign) *
    (attempt<std::string>(multi("0b") * mantissa_digits(2)) + attempt<std::string>(multi("0q") * mantissa_digits(4)) +
     attempt<std::string>(multi("0o") * mantissa_digits(8)) + attempt<std::string>(multi("0d") * mantissa_digits(10)) +
     attempt<std::string>(multi("0x") * mantissa_digits(16)) + mantissa_digits(10));
const inline auto exponent = list("eE") * option(sign) * escaped_digits(10);
const inline auto real = mantissa * option(exponent);

// boolean
const inline auto boolean = multi_list({"true", "false"});

// atoms(others)
const inline auto text =
    attempt<std::string>(bracket(multi("\"\"\""), escaped_char, attempt<std::string>(multi("\"\"\"")))) +
    bracket(one('"'), escaped_char, one('"'));

const inline auto comment = attempt<std::string>(bracket(multi("//"), any, newline + eof)) +
                            bracket(multi("/*"), any, attempt<std::string>(multi("*/")));
const inline auto variable = (alpha + one('_')) * many0(alnum + one('_'));
const inline auto character = one('\'') * escaped_char * one('\'');
} // namespace tokenize::parsers

#include "parsers.cxx"
