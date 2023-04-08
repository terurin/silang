#include "readers.hpp"

namespace tokenize::readers {

position::position(size_t _offset, size_t _line, size_t _number) : offset(_offset), line(_line), number(_number) {}

const position &position::operator=(const position &p) {
    offset = p.offset, line = p.line, number = p.number;
    return *this;
}

bool position::operator==(const position &p) const {
    return offset == p.offset && line == p.line && number == p.number;
}

bool position::operator!=(const position &p) const {
    return offset != p.offset || line != p.line || number != p.number;
}

void position::next(char c) {
    offset += 1;
    if (c != '\n' && c != '\r') {
        line += 0, number += 1;
    } else {
        line += 1, number = 0;
    }
}

string_reader::string_reader(std::string_view _body)
    : body(_body), begin(body.begin()), iter(body.begin()), end(body.end()) {}

std::optional<char> string_reader::peek() const {
    if (iter == end) {
        return std::nullopt;
    }
    return *iter;
}

std::optional<char> string_reader::next() {
    if (iter == end) {
        return std::nullopt;
    }
    pos.next(*iter);
    return *(iter++);
}

void string_reader::set_position(const position &p) {
    pos = p;
    iter = begin + p.offset;
}

} // namespace tokenize::readers