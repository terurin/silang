#include "parsers.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <unordered_set>
namespace tokenize::parsers {

atom::atom(char c) { match.set((unsigned char)c); }
atom::atom(std::initializer_list<char> items) {
    for (auto item : items) {
        match.set((unsigned char)item);
    }
}

atom::atom(std::string_view items) {
    for (auto item : items) {
        match.set((unsigned char)item);
    }
}

bool atom::operator()(reader_ptr &reader, char &c) const {
    const auto peek = reader->peek();
    if (!peek || !match.test((unsigned int)*peek)) {
        return false;
    }

    reader->next(), c = *peek;
    return true;
}

bool atom::operator()(reader_ptr &reader, std::string &s) const {
    const auto peek = reader->peek();
    if (!peek || !match.test((unsigned int)*peek)) {
        return false;
    }

    reader->next(), s.push_back(*peek);
    return true;
}

atom range(unsigned char first, unsigned char last) {
    match_t m;
    assert(first <= last);
    for (size_t i = first; i <= last; i++) {
        m.set(i);
    }
    return m;
}

atom operator+(const atom &x, const atom &y) { return atom(x.get_match() | y.get_match()); }
atom operator-(const atom &x, const atom &y) { return atom(x.get_match() & ~y.get_match()); }

// 整数関係
atom digit(unsigned int base) {
    match_t m;

    unsigned char i = 0;
    // 0~9
    for (; i < base && i < 10; i++) {
        m.set('0' + i);
    }
    // 10~
    for (; i < base; i++) {
        m.set('a' + i - 10);
        m.set('A' + i - 10);
    }
    return atom(m);
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

bool eof(reader_ptr &reader, std::string &s) { return !reader->peek(); }
bool nop(reader_ptr &reader, std::string &s) { return true; }

} // namespace tokenize::parsers