#include "tokenize.hpp"
#include <map>
namespace tokenize::tokens {
const static std::map<std::string, token_id> op_table = []() -> std::map<std::string, token_id> {
    using enum token_id;

    std::map<std::string, token_id> t;

    // assign
    t.insert({{"=", op_assign}, {":=", op_assign_bind}});
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
              {",", op_comma},
              {":", op_colon}});

    // keyword
    t.insert({{"bool", word_bool}, {"true", word_true}, {"false", word_false}});
    // int,uint
    t.insert({{"char", word_char}, {"int", word_int}, {"uint", word_uint}, {"str", word_str}, {"func", word_func}});
    return t;
}();

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
        member(character),
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

bool operation(reader_ptr &reader, token &t) {
    const static parsers::multi_list keywords([]() {
        std::vector<std::string> ks;
        for (const auto &[key, value] : op_table) {
            ks.push_back(key);
        }

        return ks;
    }());

    std::string s;
    const auto pos = reader->get_position();
    if (!keywords(reader, s)) {
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
    using namespace parsers;
    std::string s;

    many0(spaces + comment)(reader, s);

    // operation
    if (operation(reader, token)) {
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
    if (tokener(token_id::text, attempt(text))(reader, token)) {
        return true;
    }
    // '
    if (tokener(token_id::character, attempt(character))(reader, token)) {
        return true;
    }

    // [a-zA-Z_]
    if (tokener(token_id::variable, variable)(reader, token)) {
        return true;
    }
    return false;
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

} // namespace tokenize::tokens