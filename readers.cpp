#include "readers.hpp"

namespace readers {

position::position(std::string_view _name, size_t _offset, size_t _line, size_t _number)
    : name(_name), offset(_offset), line(_line), number(_number) {}

const position &position::operator=(const position &p) {
    name = p.name, offset = p.offset, line = p.line, number = p.number;
    return *this;
}

bool position::operator==(const position &p) const {
    return name == p.name && offset == p.offset && line == p.line && number == p.number;
}

bool position::operator!=(const position &p) const {
    return name != p.name || offset != p.offset || line != p.line || number != p.number;
}

void position::next(char c) {
    offset += 1;
    if (c != '\n') {
        line += 0, number += 1;
    } else {
        line += 1, number = 0;
    }
}

string_reader::string_reader(std::string_view _name, std::string_view _body)
    : name(_name), body(_body), pos(name), begin(body.begin()), iter(body.begin()), end(body.end()) {}

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

} // namespace readers