#pragma once

#include "readers.hpp"
#include <algorithm>
#include <assert.h>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace parsers {

using readers::reader_ptr, readers::position;
using std::string;

template <class P>
concept token_parser = requires(P parser, reader_ptr &reader, char &c) {
                           { parser.parse(reader, c) } -> std::convertible_to<bool>;
                       };
template <class P>
concept token_opt_parser = requires(P parser, reader_ptr &reader, std::optional<char> &c) {
                               { parser.parse(reader, c) } -> std::convertible_to<bool>;
                           };

template <class P>
concept tokens_parser = requires(P parser, reader_ptr &reader, std::string &s) {
                            { parser.parse(reader, s) } -> std::convertible_to<bool>;
                        };

namespace token {

class signle {
    const char cmp;

public:
    constexpr signle(char _c) : cmp(_c) { static_assert(token_parser<signle>); }
    bool parse(reader_ptr &, char &) const;
};

class list {
    const std::vector<char> cmps;

public:
    list(const std::vector<char> &_cmps) : cmps(_cmps) { static_assert(token_parser<list>); }
    list(std::initializer_list<char> _cmps) : cmps(_cmps) { static_assert(token_parser<list>); }
    bool parse(reader_ptr &, char &) const;
};

class range {
    const char begin, end;

public:
    constexpr range(char _begin, char _end) : begin(_begin), end(_end) { static_assert(token_parser<range>); }
    bool parse(reader_ptr &, char &) const;
};

class satify {
    const std::function<bool(char)> pred;

public:
    satify(const std::function<bool(char)> &_p) : pred(_p) {}
    bool parse(reader_ptr &, char &) const;
};

std::optional<int> base_number(char c);
class digit_base {
    const int base;

public:
    constexpr digit_base(int _base) : base(_base) { assert(base <= 32); }
    bool parse(reader_ptr &, char &) const;
};

template <token_parser R, token_parser L> class sum {
    R r;
    L l;

public:
    sum(const R &_r, const L &_l) : r(_r), l(_l) { static_assert(token_parser<sum>); }
    bool parse(reader_ptr &reader, char &c) const;
};

template <token_parser R, token_parser L> token::sum<R, L> operator||(const R &r, const L &l) {
    return token::sum<R, L>(r, l);
}

template <token_parser R, token_parser L> class meet {
    R r;
    L l;

public:
    meet(const R &_r, const L &_l) : r(_r), l(_l) {}
    bool parse(reader_ptr &reader, char &c) const;
};

template <token_parser R, token_parser L> token::meet<R, L> operator&&(const R &r, const L &l) {
    return token::meet<R, L>(r, l);
}

template <token_parser P> class invert {
    P p;

public:
    invert(const P &_p) : p(_p) {}
    bool parse(reader_ptr &reader, char &c) const;
};

template <token_parser P> token::invert<P> operator!(const P &p) { return token::invert<P>(p); }

const inline auto sign = list({'+', '-'});
const inline auto newline = list({'\n', '\r'});
const inline auto bin = range('0', '1' + 1);
const inline auto oct = range('0', '7' + 1);
const inline auto digit = range('0', '9' + 1);
const inline auto hex = digit || range('a', 'f' + 1) || range('A', 'F' + 1);
const inline auto alpha = range('a', 'z' + 1) || range('A', 'Z' + 1);
const inline auto alnum = sum(digit, alpha);
const inline auto space = list({' ', '\t', '\n', '\r'});
const inline auto base = list({'b', 'q', 'o', 'd', 'x'});
const inline auto any = satify([](char c) { return true; });

template <token_parser P> class opt {
    P parser;

public:
    opt(const P &_parser) : parser(_parser) {}
    bool parse(reader_ptr &reader, std::optional<char> &c) const;
};

} // namespace token

namespace tokens {

class serial {
    const std::string cmp;

public:
    serial(const std::string &_cmp) : cmp(_cmp) {}
    bool parse(reader_ptr &, std::string &) const;
};

class from_function {
    std::function<bool(reader_ptr &, std::string &)> func;

public:
    from_function(const std::function<bool(reader_ptr &, std::string &)> &_func) : func(_func) {}
    bool parse(reader_ptr &r, std::string &s) const { return func(r, s); };
};

template <token_parser P> class from_token {
    P parser;

public:
    from_token(const P &_parser) : parser(_parser) {}
    bool parse(reader_ptr &, std::string &) const;
};

template <token_opt_parser P> class from_token_opt {
    P parser;

public:
    from_token_opt(const P &_parser) : parser(_parser) {}
    bool parse(reader_ptr &, std::string &) const;
};

template <tokens_parser P> class many0 {
    P parser;

public:
    many0(const P &_parser) : parser(_parser) {}
    bool parse(reader_ptr &, std::string &) const;
};

template <tokens_parser P> class many1 {
    P parser;

public:
    many1(const P &_parser) : parser(_parser) {}
    bool parse(reader_ptr &, std::string &) const;
};

template <tokens_parser R, tokens_parser L> class chain2 {
    R r;
    L l;

public:
    chain2(const R &_r, const L &_l) : r(_r), l(_l) {}
    bool parse(reader_ptr &, std::string &) const;
};

template <tokens_parser P> class attempt {
    P parser;

public:
    attempt(const P &_parser) : parser(_parser) {}
    bool parse(reader_ptr &, std::string &) const;
};

template <tokens_parser R, tokens_parser L> class mixer {
    R r;
    L l;

public:
    mixer(const R &_r, const L &_l) : r(_r), l(_l) {}
    bool parse(reader_ptr &, std::string &) const;
};

const inline auto spaces0 = tokens::many0(tokens::from_token(token::space));
const inline auto spaces1 = tokens::many1(tokens::from_token(token::space));
const inline auto digits0 = tokens::many0(tokens::from_token(token::digit));
const inline auto digits1 = tokens::many1(tokens::from_token(token::digit));

static inline auto digits0_base(unsigned int n) { return tokens::many0(tokens::from_token(token::digit_base(n))); }

static inline auto digits1_base(unsigned int n) { return tokens::many1(tokens::from_token(token::digit_base(n))); }

bool integer_parse(reader_ptr &, std::string &);
bool real_parse(reader_ptr &, std::string &);
bool variable_parse(reader_ptr &, std::string &);
bool comment_parse(reader_ptr &, std::string &);
const inline auto integer = tokens::from_function(integer_parse);
const inline auto real = tokens::from_function(real_parse);
const inline auto variable = tokens::from_function(variable_parse); 
const inline auto comment = tokens::from_function(comment_parse);
} // namespace tokens

namespace words {

enum class word_id {};
struct word {
    std::string text;
};

}; // namespace words

} // namespace parsers

#include "parsers.tpp"
