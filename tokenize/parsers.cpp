#include "parsers.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <unordered_set>
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
    {
        const position pos = reader->get_position();
        for (; count < min; count++) {
            if (!parser(reader, s)) {
                if (pos != reader->get_position()) {
                    std::cerr << "overrun (repeat min)" << std::endl;
                }
                return false;
            }
        }
    }

    // ~max
    for (; count < max; count++) {
        const position pos = reader->get_position();
        if (!parser(reader, s)) {
            if (pos != reader->get_position()) {
                std::cerr << "overrun (repeat max)" << std::endl;
            }
            return true;
        }
    }
    return true;
}

bool attempt::operator()(reader_ptr &reader, std::string &s) const {
    // store
    const size_t length_keep = s.length();
    const position p_keep = reader->get_position();
    // parse
    if (parser(reader, s)) {
        return true;
    }
    // restore
    s.resize(length_keep);
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
    if (keep != reader->get_position()) {
        std::cerr << "overrun" << std::endl;
        return false;
    }

    return left(reader, s);
}

bool bracket::operator()(reader_ptr &reader, std::string &out) const {
    if (!begin(reader, out)) {
        return false;
    }
    do {
        if (end(reader, out)) {
            return true;
        }
    } while (body(reader, out));

    return false;
}

satify one(char token) {
    return satify([token](char c) { return token == c; });
}

satify range(char begin, char end) {
    assert(begin <= end);
    return satify([begin, end](char c) { return begin <= c && c <= end; });
}

satify list(std::string_view _items) {
    const std::unordered_set<char> items(_items.begin(), _items.end());
    return satify([items](char c) -> bool { return items.contains(c); });
}

// 整数関係
satify digit(unsigned int base) {
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

bool eof(reader_ptr &reader, std::string &s) { return !reader->peek(); }
bool nop(reader_ptr &reader, std::string &s) { return true; }

} // namespace tokenize::parsers