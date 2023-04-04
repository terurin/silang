#include "parsers.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
namespace tokenize::parsers {

bool satify::operator()(reader_ptr &reader, char &c) const {
    const auto peek = reader->peek();
    if (!peek || !predicate(*peek)) {
        return false;
    }

    reader->next(), c = *peek;
    return true;
}

bool satify::operator()(reader_ptr &reader, std::string &s) const {
    const auto peek = reader->peek();
    if (!peek || !predicate(*peek)) {
        return false;
    }

    reader->next(), s.push_back(*peek);
    return true;
}

bool multi::operator()(reader_ptr &reader, std::string &s) const {
    for (const char c : keyword) {
        const auto peek = reader->peek();
        if (!peek || *peek != c) {
            return false;
        }
        reader->next(), s.push_back(*peek);
    }

    return true;
}

static inline std::unordered_map<std::string, bool> table_from_keywords(const std::vector<std::string> &keywords) {
    std::unordered_map<std::string, bool> table;
    // 部分文字列を書き出す
    for (const std::string &keyword : keywords) {
        for (size_t i = 0; i < keyword.length(); i++) {
            table[keyword.substr(0, i)] = false;
        }
    }
    // 完全一致文字列を書き出す
    for (const std::string &keyword : keywords) {
        table[keyword] = true;
    }

    return table;
}

multi_list::multi_list(const std::vector<std::string> &_keywords) : table(table_from_keywords(_keywords)) {}

bool multi_list::operator()(reader_ptr &reader, std::string &s) const {

    bool is_rollbackable = false;
    std::string rollback_string;
    position rollback_position;

    std::string match;
    do {
        bool is_failed;
        do {
            auto peek = reader->peek();
            if (!peek) {
                is_failed = true;
                break;
            }
            match.push_back(*peek);

            auto iter = table.find(match);
            if (iter == table.end()) {
                is_failed = true;
                break;
            }

            reader->next();
            if (iter->second) {
                // save as rollback
                is_rollbackable = true;
                rollback_position = reader->get_position();
                rollback_string = match;
            }
            is_failed = false;
        } while (0);

        if (is_failed) {
            if (!is_rollbackable) {
                return false;
            }
            reader->set_position(rollback_position);
            s = rollback_string;
            return true;
        }

    } while (1);

    return false;
}

bool chain::operator()(reader_ptr &reader, std::string &s) const {

    if (!right(reader, s)) {
        return false;
    }

    return left(reader, s);
}

repeat_range::repeat_range(const parser_t<std::string> &_parser, unsigned int _min, unsigned int _max)
    : parser(_parser), min(_min), max(_max) {
    assert(min <= max);
}

bool repeat_range::operator()(reader_ptr &reader, std::string &s) const {
    int count = 0;
    // min
    for (; count < min; count++) {
        if (!parser(reader, s)) {
            return false;
        }
    }
    // ~max
    for (; count < max; count++) {
        if (!parser(reader, s)) {
            return true;
        }
    }
    return true;
}

bool attempt::operator()(reader_ptr &reader, std::string &s) const {
    // store
    std::string s_keep = s;
    const position p_keep = reader->get_position();
    // parse
    if (parser(reader, s)) {
        return true;
    }
    // restore
    s = s_keep;
    reader->set_position(p_keep);
    return false;
}

bool sum::operator()(reader_ptr &reader, std::string &s) const {
    // store
    const auto keep = reader->get_position();

    if (right(reader, s)) {
        return true;
    }
    // error check
    if (reader->get_position() != reader->get_position()) {
        std::cerr << "overrun" << std::endl;
        return false;
    }

    return left(reader, s);
}

satify one(char token) {
    return satify([token](char c) { return token == c; });
}

satify range(char begin, char end) {
    return satify([begin, end](char c) { return begin <= c && c <= end; });
}

satify list(std::initializer_list<char> _items) {
    const std::vector<char> items(_items);
    return satify([items](char c) -> bool {
        for (char item : items) {
            if (item == c) {
                return true;
            }
        }
        return false;
    });
}

satify list(std::string_view _items) {
    const std::string items(_items);
    return satify([items](char c) -> bool {
        for (char item : items) {
            if (item == c) {
                return true;
            }
        }
        return false;
    });
}

// 整数関係
satify digit_base(unsigned int base) {
    assert(base <= 10 + 26);
    if (base <= 10) {
        return range('0', '0' + base - 1);
    }

    return satify([base](char c) {
        bool result = false;
        result |= '0' <= c && c <= '9';
        result |= 'a' <= c && c <= 'a' + base - 0xa - 1;
        result |= 'A' <= c && c <= 'A' + base - 0xa - 1;
        return result;
    });
}

std::optional<unsigned int> base_number(char c) {
    switch (c) {
    case 'b':
        return 2;
    case 'q':
        return 4;
    case 'o':
        return 8;
    case 'd':
        return 10;
    case 'x':
        return 16;
    default:
        return std::nullopt;
    }
}

bool integer(reader_ptr &reader, std::string &s) {
    // [-+]?
    if (!option(sign)(reader, s)) {
        return false;
    }
    // 0[base][digit]
    char c;
    if (one('0')(reader, c)) {
        s.push_back(c);
        if (base_list(reader, c)) {
            s.push_back(c);
            const unsigned int base = base_number(c).value();
            return many1(digit_base(base))(reader, s);
        } else {
            return many0(digit_dec)(reader, s);
        }
    } else {
        return many1(digit_dec)(reader, s);
    }
}

bool real(reader_ptr &reader, std::string &s) {
    // [-+]?
    if (!option(sign)(reader, s)) {
        return false;
    }
    // 0[base][digit]
    char c;
    int base = 10;
    if (one('0')(reader, c)) {
        s.push_back(c);
        if (base_list(reader, c)) {
            s.push_back(c);
            base = base_number(c).value();
        }
    }
    if (!many0(digit_base(base))(reader, s)) {
        return false;
    }

    if (!list(".eE")(reader, c)) {
        return false;
    }
    s.push_back(c);
    if (c == '.') {
        if (!many1(digit_base(base))(reader, s)) {
            return false;
        }
        if (!list("eE")(reader, s)) {
            return true;
        }
        s.push_back(c);
    }

    // [-+]?
    if (!option(sign)(reader, s)) {
        return false;
    }
    return many1(digit_dec)(reader, s);
}

bool eof(reader_ptr &reader, std::string &s) { return !reader->peek(); }
bool nop(reader_ptr &reader, std::string &s) { return true; }

bool text(reader_ptr &reader, std::string &s) {
    if (one('"')(reader, s)) {
        if (!many0(satify([](char c) -> bool { return c != '"'; }))(reader, s)) {
            return false;
        }
        return one('"')(reader, s);
    }
    return false;
}

bool comment(reader_ptr &reader, std::string &s) {
    if (attempt(multi("//"))(reader, s)) {
        if (!many0(satify([](char c) { return c != '\n' && c != '\r'; }))(reader, s)) {
            return false;
        };
        return (newline + eof)(reader, s);
    }
    if (attempt(multi("/*"))(reader, s)) {
        do {
            if (attempt(multi("*/"))(reader, s)) {
                return true;
            }
        } while (any(reader, s));
    }
    return false;
}

bool variable(reader_ptr &reader, std::string &s) {
    // [a-zA-Z_][0-9a-zA-Z_]*
    if (!(alpha + one('_'))(reader, s)) {
        return false;
    }
    return many0(alnum + one('_'))(reader, s);
}

bool character(reader_ptr &reader, std::string &s) {
    // '*'
    return (one('\'') * option(one('\\')) * any * one('\''))(reader, s);
}

} // namespace tokenize::parsers