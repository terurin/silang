#include "parsers.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <stack>
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
    if (is_epsilon()) {
        return true;
    }

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

const atom atom::epsilon;

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
    assert(base <= 10 + 26); // sum of digits and alphabets
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

std::optional<size_t> instruction::parse(reader_ptr &reader) const {
    using std::nullopt;

    std::optional<size_t> result = nullopt;
    if (is_epsilon()) {
        result = 0;
    }

    const auto peek = reader->peek();
    if (!peek) {
        return result;
    }

    if (match.test(*peek) && accept) {
        result = 1;
    }
    if (match.test(*peek) && success) {
        const auto position = reader->get_position();
        reader->next();
        if (const auto read = success->parse(reader); read) {
            result = std::max(result.value_or(0), *read + 1);
        }
        reader->set_position(position);
    }
    if (next) {
        const auto position = reader->get_position();
        if (const auto read = next->parse(reader); read) {
            result = std::max(result.value_or(0), *read);
        }
        reader->set_position(position);
    }
    return result;
}

beaker::beaker() {
    instruction *const inst = new instruction(atom::epsilon);
    inst->set_accept();
    owners.push_back(inst);
    root = inst;
}

beaker::beaker(const atom &a) {
    instruction *const inst = new instruction(a);
    inst->set_accept();

    owners.push_back(inst);
    root = inst;
}

beaker::beaker(const beaker &b) : beaker(std::move(b.clone())) {}

beaker::~beaker() {
    for (auto &owner : owners) {
        delete owner;
        owner = nullptr;
    }
    owners.clear();
}

bool beaker::operator()(reader_ptr &reader, std::string &out) {
    assert(root);
    const auto read = root->parse(reader);
    if (!read) {
        return false;
    }

    for (size_t s = 0; s < *read; s++) {
        auto next = reader->next();
        if (!next) {
            std::cerr << "unexpected read" << std::endl;
            return false;
        }
        out.push_back(*next);
    }
    return true;
}

beaker::beaker(beaker &&origin) {
    std::swap(owners, origin.owners);
    std::swap(root, origin.root);
}

beaker beaker::clone() const {
    assert(root);
    std::vector<instruction *> freshes;
    freshes.reserve(owners.size());
    std::unordered_map<const instruction *, instruction *> table;

    // allocate
    for (const instruction *stale : owners) {
        instruction *fresh = new instruction(*stale);
        table[stale] = fresh;
        freshes.push_back(fresh);
    }

    // convert pointer
    for (instruction *fresh : freshes) {
        if (fresh->next) {
            fresh->next = table[fresh->next];
        }
        if (fresh->success) {
            fresh->success = table[fresh->success];
        }
    }
    return beaker(std::move(freshes), table[root]);
}

beaker beaker::option(beaker &&x) {
    std::vector<instruction *> owners;
    std::swap(owners, x.owners);

    // insert
    instruction *const inst = new instruction(atom::epsilon);
    inst->set_accept();
    inst->next = x.root;
    owners.push_back(inst);

    return beaker(std::move(owners), inst);
}

beaker beaker::chain(beaker &&x, beaker &&y) {
    std::vector<instruction *> owners;
    owners.reserve(2 * x.owners.size() + y.owners.size());

    // xで受理できるものをyのrootに変更する
    for (instruction *inst : x.owners) {
        if (!inst->accept) continue;
        inst->accept = false;
        if (!inst->success) {
            inst->success = y.root;
            continue;
        }

        // insert
        instruction *const inst2 = new instruction(*inst); // copy
        inst->next = inst2;
        inst2->success = y.root;
        owners.push_back(inst2);
    }
    // transfer ownerships
    instruction *root = x.root;
    std::copy(x.owners.begin(), x.owners.end(), std::back_inserter(owners));
    std::copy(y.owners.begin(), y.owners.end(), std::back_inserter(owners));
    x.owners.clear(), x.root = nullptr;
    y.owners.clear(), y.root = nullptr;

    return beaker(std::move(owners), root);
}

beaker beaker::text(std::string_view sv) {
    assert(sv.length() > 0);
    std::vector<instruction *> owners;
    owners.reserve(sv.length());

    instruction *last = nullptr;
    for (auto iter = sv.rbegin(); iter != sv.rend(); iter++) {
        instruction *now = new instruction(atom(*iter));
        now->set_success(last);
        owners.push_back(now);
        last = now;
    }
    owners[0]->accept = true;

    return beaker(std::move(owners), last);
}

beaker beaker::many0(beaker &&base) {
    std::vector<instruction *> owners;

    std::swap(owners, base.owners);
    owners.reserve(2 * owners.size()); // 最悪値
    for (instruction *owner : owners) {
        if (!owner->accept) continue;
        if (!owner->success) {
            owner->success = base.root;
            continue;
        }
        // insert
        instruction *insert = new instruction(*owner);
        insert->set_accept(false);
        insert->set_success(base.root);
        insert->set_next(owner->next);
        owner->next = insert;
    }

    instruction *root = new instruction(atom::epsilon);
    root->set_accept();
    root->next = base.root;
    base.root = nullptr;
    return beaker(std::move(owners), root);
}

beaker beaker::many1(beaker &&base) {
    std::vector<instruction *> owners;

    std::swap(owners, base.owners);
    owners.reserve(2 * owners.size()); // 最悪値
    for (instruction *owner : owners) {
        if (!owner->accept) continue;
        if (!owner->success) {
            owner->success = base.root;
            continue;
        }
        // insert
        instruction *insert = new instruction(*owner);
        insert->set_accept(false);
        insert->set_success(base.root);
        insert->set_next(owner->next);
        owner->next = insert;
    }

    instruction *const root = base.root;
    base.root = nullptr;
    return beaker(std::move(owners), root);
}

beaker beaker::sum(beaker &&x, beaker &&y) {
    std::vector<instruction *> owners;
    // transfer ownerships
    std::swap(owners, x.owners);
    owners.reserve(owners.size() + y.owners.size());
    std::copy(y.owners.begin(), y.owners.end(), std::back_inserter(owners));

    instruction *root = x.root;

    // merge
    // TODO: 重複を削除する機能を追加する
    instruction *last = root;
    for (instruction *iter = root->next; iter != nullptr; iter = iter->next) {
        last=iter;
    }
    last->next= y.root;

    y.owners.clear();
    y.root = nullptr;

    return beaker(std::move(owners), root);
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