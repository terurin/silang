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
    match_t match; // if match is 0 then epsilon

public:
    atom() {}
    atom(char);
    atom(std::initializer_list<char>);
    atom(std::string_view);
    atom(const match_t &_match) : match(_match) {}
    atom(const atom &) = default;
    atom(atom &&) = default;
    bool is_epsilon() const { return match.none(); }
    const match_t get_match() const { return match; }

    bool operator()(reader_ptr &, char &) const;
    bool operator()(reader_ptr &, std::string &) const;
    const static atom epsilon;
    bool test(unsigned char x) const { return match.test(x); }
};

// 生成関係
atom range(unsigned char first, unsigned char last);
static inline atom one(unsigned char c) { return atom(c); }
static inline atom list(std::string_view sv) { return atom(sv); }

inline atom any = range(0, 255);
atom operator+(const atom &, const atom &);
atom operator-(const atom &, const atom &);

// 記号
const inline atom sign = atom("+-");
const inline atom escape = atom({'\\'});
const inline atom not_escape = any - escape;
atom digit(unsigned int n = 10);
const inline atom small = range('a', 'z');
const inline atom large = range('A', 'Z');
const inline atom alpha = small + large;
const inline atom alnum = small + large + digit();
// 空白
const inline auto newline = list("\r\n");
const inline auto space = list(" \t\n\r");

// 内部表現

//
struct flask {
    atom inner;
    bool accept = false;
    int color = 0;
    flask *success = nullptr;
    flask *next = nullptr;

    flask(const atom &_inner) : inner(_inner) {}
    flask(match_t _match) : inner(_match) {}
    flask(const flask &) = default;

    flask &set_accept(bool _accept = true) { return accept = _accept, *this; }
    flask &set_color(int _color = true) { return color = color, *this; }
    flask &set_success(flask *_success) { return success = _success, *this; }
    flask &set_next(flask *_next) { return next = _next, *this; }
    bool is_epsilon() const { return inner.is_epsilon(); }

    std::optional<size_t> parse(reader_ptr &) const;
};

class beaker {
    std::vector<flask *> owners;
    flask *root = nullptr;

public:
    beaker(std::vector<flask *> &&_owners, flask *_root) ;
    beaker();
    beaker(const atom &);
    beaker(const beaker &); // 暫定対応、後々deleteに戻す
    beaker(beaker &&);
    ~beaker();
    bool operator()(reader_ptr &, std::string &);

    beaker clone() const; // O(n)
    static beaker option(beaker &&);
    static beaker chain(beaker &&, beaker &&);
    static beaker text(std::string_view);
    static beaker many0(beaker &&);
    static beaker many1(beaker &&);
    static beaker sum(beaker &&, beaker &&);
};

// 整数関係
beaker escaped_digits(unsigned int n = 10);
beaker escaped_char();
beaker spaces();

// alias
// static inline beaker option(beaker &&x) { return beaker::option(std::move(x)); }
// static inline beaker chain(beaker &&x, beaker &&y) { return beaker::chain(std::move(x), std::move(y)); }
// static inline beaker text(std::string_view sv) { return beaker::text(sv); }
// static inline beaker many0(beaker &&x) { return beaker::many0(std::move(x)); }
// static inline beaker many1(beaker &&x) { return beaker::many1(std::move(x)); }

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

// 特殊
bool eof(reader_ptr &, std::string &);
bool nop(reader_ptr &, std::string &);

// integer
const inline auto integer =
    option(sign) * (attempt<std::string>(beaker::text("0b") * escaped_digits(2)) +
                    attempt<std::string>(beaker::text("0q") * escaped_digits(4)) +
                    attempt<std::string>(beaker::text("0o") * escaped_digits(8)) +
                    attempt<std::string>(beaker::text("0d") * escaped_digits(10)) +
                    attempt<std::string>(beaker::text("0x") * escaped_digits(16)) + escaped_digits(10));

// real
const inline auto dot = one('.');
const inline auto mantissa_digits(unsigned int n) { return escaped_digits(n) * dot * escaped_digits(n); }
const inline auto mantissa =
    option(sign) * (attempt<std::string>(beaker::text("0b") * mantissa_digits(2)) +
                    attempt<std::string>(beaker::text("0q") * mantissa_digits(4)) +
                    attempt<std::string>(beaker::text("0o") * mantissa_digits(8)) +
                    attempt<std::string>(beaker::text("0d") * mantissa_digits(10)) +
                    attempt<std::string>(beaker::text("0x") * mantissa_digits(16)) + mantissa_digits(10));
const inline auto exponent = list("eE") * option(sign) * escaped_digits(10);
const inline auto real = mantissa * option(exponent);

// boolean
const inline auto boolean = multi_list({"true", "false"});

// atoms(others)
const inline auto text = attempt<std::string>(bracket(beaker::text("\"\"\""), escaped_char(),
                                                      attempt<std::string>(beaker::text("\"\"\"")))) +
                         bracket(one('"'), escaped_char(), one('"'));

const inline auto comment = attempt<std::string>(bracket(beaker::text("//"), any, newline + eof)) +
                            bracket(beaker::text("/*"), any, attempt<std::string>(beaker::text("*/")));
const inline auto variable = (alpha + one('_')) * many0(alnum + one('_'));
const inline auto character = one('\'') * escaped_char() * one('\'');
} // namespace tokenize::parsers

#include "parsers.cxx"
