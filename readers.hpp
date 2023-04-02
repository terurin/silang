#pragma once
#include <memory>
#include <optional>
#include <stddef.h>
#include <string>
#include <string_view>
namespace readers {

struct position final {
    std::string name;
    size_t offset;
    size_t line, number;

    position(std::string_view _name = "", size_t _offset = 0, size_t _line = 0, size_t _number = 0);
    position(const position &p) = default;
    position(position &&p) = default;
    ~position() = default;

    const position &operator=(const position &p);
    bool operator==(const position &p) const;
    bool operator!=(const position &p) const;
    void next(char c);
};

struct reader {
    virtual std::optional<char> peek() const = 0;
    virtual std::optional<char> next() = 0;
    virtual const position &get_position() const = 0;
    virtual void set_position(const position &) = 0;
};

using reader_ptr = std::shared_ptr<reader>;
using char_opt = std::optional<char>;

class string_reader : public reader {
    std::string name;
    std::string body;
    position pos;
    const std::string::const_iterator begin, end;
    std::string::const_iterator iter;

public:
    string_reader(std::string_view _name, std::string_view _body);
    string_reader(const string_reader &sr) = default;
    string_reader(string_reader &&sr) = default;
    virtual ~string_reader() = default;

    virtual std::optional<char> peek() const override;
    virtual std::optional<char> next();
    virtual const position &get_position() const override { return pos; }
    virtual void set_position(const position &p) override;
};

static inline reader_ptr make_string_reader(std::string_view name, std::string_view src) {
    return std::dynamic_pointer_cast<reader>(std::make_shared<string_reader>(name, src));
}

} // namespace readers
