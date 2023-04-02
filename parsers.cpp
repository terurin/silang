#include "parsers.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
namespace parsers {
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

std::vector<std::string> multi_list::keywords_sort(const std::vector<std::string> &_keywords) {

    std::vector<std::string> keywords;
    keywords.reserve(_keywords.size());
    std::copy(_keywords.begin(), _keywords.end(), std::back_inserter(keywords));
    std::sort(keywords.begin(), keywords.end(),
              [](const std::string &a, const std::string &b) { return a.length() > b.length(); });
    return keywords;
}

bool multi_list::operator()(reader_ptr &reader, std::string &s) const {

    // store
    const position keep_pos = reader->get_position();
    std::string keep_s = s;

    // match
    for (const auto keyword : keywords) {
        for (const char c : keyword) {
            const auto peek = reader->peek();
            if (!peek || *peek != c) {
                goto next;
            }
            reader->next(), s.push_back(*peek);
        }
        return true;
    next:
        reader->set_position(keep_pos);
        s = keep_s;
    }

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
        if (many0(satify([](char c) { return c != '\n' && c != '\n'; }))(reader, s)) {
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

const static std::map<std::string, token_id> op_table = []() -> std::map<std::string, token_id> {
    using enum token_id;

    std::map<std::string, token_id> t;

    // assign
    t.insert({{"=", op_assign}});
    // arith
    t.insert({{"+", op_add}, {"-", op_sub}, {"*", op_mul}, {"/", op_div}, {"%", op_mod}});
    t.insert({
        {"+=", op_assign_add},
        {"-=", op_assign_sub},
        {"*=", op_assign_mul},
        {"/=", op_assign_div},
        {"%=", op_assign_mod},
    });

    // compare
    t.insert({{"==", op_eq}, {"=!", op_ne}, {">", op_gt}, {">=", op_ge}, {"<", op_lt}, {"<=", op_le}});

    // bit logic
    t.insert(
        {{"~", op_bitnot}, {"&", op_bitand}, {"|", op_bitor}, {"^", op_bitxor}, {">>", op_rshfit}, {"<<", op_lshfit}});
    t.insert({
        {"&=", op_assign_bitand},
        {"|=", op_assign_bitor},
        {"^=", op_assign_bitxor},
        {">>=", op_assign_rshfit},
        {"<<=", op_assign_lshfit},
    });

    // logic
    t.insert({{"!", op_not}, {"&&", op_and}, {"||", op_or}, {"^^", op_xor}});
    t.insert({{"&&=", op_assign_and}, {"||=", op_assign_or}, {"^^=", op_assign_xor}});

    // member
    t.insert({{".", op_member}, {"->", op_arrow}, {"@", op_at}});

    // type
    t.insert({{"?", op_option}});

    // bracket
    t.insert({{"(", op_bracket_begin},
              {")", op_bracket_end},
              {"()", op_bracket_empty},
              {"{", op_block_begin},
              {"}", op_block_end},
              {"[", op_index_begin},
              {"]", op_index_end},
              {";", op_line},
              {",", op_comma}});

    // keyword
    t.insert({{"bool", word_bool}, {"true", word_true}, {"false", word_false}});
    // int,uint
    t.insert({{"int", word_int}, {"uint", word_uint}, {"str", word_str}});
    return t;
}();

bool operations(reader_ptr &reader, std::string &s) {

    const static multi_list keywords([]() {
        std::vector<std::string> ks;
        for (const auto &[key, value] : op_table) {
            ks.push_back(key);
        }

        return ks;
    }());

    return keywords(reader, s);
}
////////////////////////////////////////////////////////////////////////////////
//// token /////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

std::ostream &operator<<(std::ostream &os, token_id id) {
#define member(x)                                                                                                      \
    { token_id::x, #x }

    const static std::map<token_id, const char *> table{
        // error
        member(none),
        // literal
        member(boolean),
        member(integer),
        member(real),
        member(text),
        member(variable),
    };
#undef member
    char buffer[256]; // std::formatはまだまともに使えないので

    if (const auto iter = table.find(id); iter != table.end()) {
        snprintf(buffer, sizeof(buffer), "%s(%x)", iter->second, (int)id);
        return os << buffer;
    }

    for (const auto &[key, value] : op_table) {
        if (value == id) {
            snprintf(buffer, sizeof(buffer), "op[%s](%x)", key.c_str(), (int)id);
            return os << buffer;
        }
    }

    snprintf(buffer, sizeof(buffer), "unknown(%x)", (int)id);
    return os << buffer;
}

std::ostream &operator<<(std::ostream &os, const token &t) { return os << t.id << ":" << t.text; }

bool tokener::operator()(reader_ptr &reader, token &t) const {
    const position pos = reader->get_position();
    std::string text;

    if (!parser(reader, text)) {
        return false;
    }

    // update
    t.id = id;
    t.pos = pos;
    t.text = text;

    return true;
}

bool tokenize(reader_ptr &reader, token &token) {
    std::string s;
    many0(spaces + comment)(reader, s);

    // operations
    if (tokenize_op(reader, token)) {
        return true;
    }

    // [digit]
    if (tokener(token_id::real, attempt(real))(reader, token)) {
        return true;
    }
    if (tokener(token_id::integer, attempt(integer))(reader, token)) {
        return true;
    }

    // "
    if (tokener(token_id::text, text)(reader, token)) {
        return true;
    }

    // [a-zA-Z_]
    if (tokener(token_id::variable, variable)(reader, token)) {
        return true;
    }
    return false;
}

bool tokenize_op(reader_ptr &reader, token &t) {
    std::string s;
    const auto pos = reader->get_position();
    if (!operations(reader, s)) {
        return false;
    }
    // map
    t.id = token_id::none;
    if (auto iter = op_table.find(s.c_str()); iter != op_table.end()) {
        t.id = iter->second;
    }
    if (t.id == token_id::none) {
        std::cerr << "faield to convert token id:" << s << std::endl;
        return false;
    }
    t.text = s;
    t.pos = pos;
    return true;
}

bool tokenize_all(reader_ptr &reader, std::vector<token> &ts) {
    token t;
    do {
        if (!tokenize(reader, t)) {
            break;
        }
        ts.push_back(t);
    } while (1);
    return true;
}

std::ostream &operator<<(std::ostream &os, const std::vector<token> &ts) {
    auto iter = ts.begin();
    if (iter == ts.end()) {
        return os;
    }
    std::cout << *iter++;
    for (; iter != ts.end(); iter++) {
        std::cout << std::endl << *iter;
    }
    return os;
}

} // namespace parsers