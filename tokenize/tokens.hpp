#pragma once

#include "parsers.hpp"
#include "readers.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
namespace tokenize::tokens {
using parsers::parser_t;
using readers::reader_ptr, readers::position;

// tokens
enum class token_id {
    none = 0,
    // literal
    boolean = 0x10,
    integer,
    real,
    text,
    character,
    variable,
    // operations
    op_assign = 0x20, // =
    op_assign_bind,   //:=
    // arith
    op_add = 0x30, //+
    op_sub,        //-
    op_mul,        //*
    op_div,        ///
    op_mod,        //%
    op_assign_add, // +=
    op_assign_sub, // -=
    op_assign_mul, // *=
    op_assign_div, // /=
    op_assign_mod, // %=
    // compare
    op_eq = 0x40, //==
    op_ne,        //!=
    op_gt,        // >
    op_ge,        //>=
    op_lt,        //<
    op_le,        //<=
    // bit logic && shift
    op_bitnot = 0x50, //~
    op_bitand,        //&
    op_bitor,         //|
    op_bitxor,        //^
    op_rshfit,        //>>
    op_lshfit,        //<<
    op_assign_bitand, //&=
    op_assign_bitor,  //|=
    op_assign_bitxor, //^=
    op_assign_rshfit, //>>=
    op_assign_lshfit, //<<=

    // bits
    op_not = 0x60, //!
    op_and,        //&&
    op_or,         //||
    op_xor,        //^^
    op_assign_and, //&&=
    op_assign_or,  //||=
    op_assign_xor, //^^=
    // member
    op_member = 0x70, //.
    op_arrow,         //->
    op_at,            //@
    // type opration
    op_option = 0x80, //?

    // bracket& separator
    op_bracket_begin = 0xA0, //(
    op_bracket_end,          //)
    op_bracket_empty,        //()
    op_block_begin,          //{
    op_block_end,            //}
    op_index_begin,          //[
    op_index_end,            //]
    op_line,                 //;
    op_comma,                //,
    op_colon,                //:

    // type
    type_bool = 0x100,
    type_int,
    type_uint,
    type_char,
    type_str,
    type_func,
};

std::ostream &operator<<(std::ostream &, token_id);
struct token {
    token_id id = token_id::none;
    position pos;
    std::string text;
};

std::ostream &operator<<(std::ostream &, const token &);

class token_table {
    std::unordered_map<std::string, token_id> table;
    parsers::multi_list list;

public:
    token_table(const std::unordered_map<std::string, token_id> &_table);
    bool operator()(reader_ptr &, token &) const;
};

extern const token_table operations;
extern const token_table types;

class tokener {
    const token_id id;
    const parser_t<std::string> parser;

public:
    tokener(const token_id _id, parser_t<std::string> _parser) : id(_id), parser(_parser) {}
    bool operator()(reader_ptr &, token &) const;
};

bool tokenize(reader_ptr &, token &);
bool tokenize_all(reader_ptr &, std::vector<token> &);

std::ostream &operator<<(std::ostream &, const std::vector<token> &);

} // namespace tokenize::tokens