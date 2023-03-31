#pragma once
#include <memory>
#include <optional>
#include <stddef.h>
#include <string>
#include <string_view>
namespace readers {

struct position final {
    size_t offset;
    size_t line, number;

    constexpr position(size_t _offset = 0, size_t _line = 0, size_t _number = 0)
        : offset(_offset), line(_line), number(_number) {}
    constexpr position(const position &) = default;
    constexpr position(position &&) = default;
    ~position() = default;

    constexpr const position &operator=(const position &p) {
        return offset = p.offset, line = p.line, number = p.number, *this;
    }
    void next(char c)  {
    offset += 1;
    if (c != '\n') {
        line += 0, number += 1;
    } else {
        line += 1, number = 0;
    }
}
};

struct reader {
    virtual std::shared_ptr<reader> clone() const = 0;
    virtual std::optional<char> peek() const = 0;
    virtual std::optional<char> next() = 0;
    virtual const position& get_position() const = 0;
};

using reader_ptr = std::shared_ptr<reader>;
using char_opt=std::optional<char>;

class string_reader : public reader {
    std::string body;
    const std::string::const_iterator begin, end;
    std::string::const_iterator iter;
    position pos;

public:
    string_reader(std::string_view _body) : body(_body), begin(body.begin()), iter(body.begin()), end(body.end()) {}
    string_reader(const string_reader &) = default;
    string_reader(string_reader &&) = default;
    virtual ~string_reader() = default;

    virtual std::shared_ptr<reader> clone() const override {
        return std::dynamic_pointer_cast<reader>(std::make_shared<string_reader>(*this));
    }
    virtual std::optional<char> peek() const override { return iter != end ? std::make_optional(*iter) : std::nullopt; }
    virtual std::optional<char> next() {
        return iter != end ? pos.next(*iter), std::make_optional(*(iter++)) : std::nullopt;
    }
    virtual const position& get_position() const { return pos; }
};

static inline reader_ptr make_string_reader(std::string_view s) {
    return std::dynamic_pointer_cast<reader>(std::make_shared<string_reader>(s));
}

} // namespace readers
