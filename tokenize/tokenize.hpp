#pragma once
#include "parsers.hpp"
#include "readers.hpp"
#include "tokens.hpp"
namespace tokenize {
// readers
using readers::make_string_reader;
using readers::reader_ptr, readers::position;
// parsers
using tokens::token, tokens::token_id;
using tokens::tokenize, tokens::tokenize_all;
} // namespace tokenize