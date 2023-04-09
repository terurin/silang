#include "tokenize.hpp"
#include <map>
namespace tokenize::tokens {
const static std::unordered_map<std::string, token_id> operations_table =
    []() -> std::unordered_map<std::string, token_id> {
    using enum token_id;

    std::unordered_map<std::string, token_id> t;

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

    return t;
}();

const static std::unordered_map<std::string, token_id> types_table = []() {
    using enum token_id;
    std::unordered_map<std::string, token_id> t;
    // types
    t.insert({{"bool", type_bool},
              {"int", type_int},
              {"uint", type_uint},
              {"char", type_char},
              {"str", type_str},
              {"func", type_func}});
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

    for (const auto &[key, value] : operations_table) {
        if (value == id) {
            snprintf(buffer, sizeof(buffer), "op[%s](%x)", key.c_str(), (int)id);
            return os << buffer;
        }
    }

    for (const auto &[key, value] : types_table) {
        if (value == id) {
            snprintf(buffer, sizeof(buffer), "type[%s](%x)", key.c_str(), (int)id);
            return os << buffer;
        }
    }

    snprintf(buffer, sizeof(buffer), "unknown(%x)", (int)id);
    return os << buffer;
}

std::ostream &operator<<(std::ostream &os, const token &t) { return os << t.id << ":" << t.text; }

token_table::token_table(const std::unordered_map<std::string, token_id> &_table)
    : table(_table), list([](const std::unordered_map<std::string, token_id> &ts) {
          std::vector<std::string> ks;
          for (const auto &[key, value] : ts) {
              ks.push_back(key);
          }

          return ks;
      }(_table)) {}

bool token_table::operator()(reader_ptr &reader, token &t) const {
    std::string s;
    const auto pos = reader->get_position();
    if (!list(reader, s)) {
        return false;
    }
    // map
    t.id = token_id::none;
    if (auto iter = table.find(s.c_str()); iter != table.end()) {
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

const token_table operations(operations_table);
const token_table types(types_table);

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

bool tokenize(reader_ptr &reader, token &t) {
    using namespace parsers;
    std::string s;

    many0(spaces + comment)(reader, s);

    // operation
    if (attempt<token>(types)(reader, t)) {
        return true;
    }
    if (attempt<token>(operations)(reader, t)) {
        return true;
    }

    // [digit]
    if (attempt<token>(tokener(token_id::real, real))(reader, t)) {
        return true;
    }

    if (attempt<token>(tokener(token_id::boolean, boolean))(reader, t)) {
        return true;
    }

    if (attempt<token>(tokener(token_id::integer, integer))(reader, t)) {
        return true;
    }

    // "
    if (attempt<token>(tokener(token_id::text, text))(reader, t)) {
        return true;
    }
    // '
    if (attempt<token>(tokener(token_id::character, character))(reader, t)) {
        return true;
    }

    // [a-zA-Z_]
    if (tokener(token_id::variable, variable)(reader, t)) {
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