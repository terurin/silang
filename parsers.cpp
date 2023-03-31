#include "parsers.hpp"
#include <algorithm>

namespace parsers {

namespace token {
bool signle::parse(reader_ptr &p, char &c) const {
    auto peek = p->peek();
    if (!peek) {
        return false;
    }
    if (*peek != cmp) {
        return false;
    }
    p->next();
    c = *peek;
    return true;
}

bool list::parse(reader_ptr &p, char &c) const {
    auto peek = p->peek();
    if (!peek) {
        return false;
    }

    auto iter = std::find(cmps.begin(), cmps.end(), *peek);
    if (iter == cmps.end()) {
        return false;
    }
    p->next();
    c = *peek;
    return true;
}

bool range::parse(reader_ptr &p, char &c) const {
    auto peek = p->peek();
    if (!peek) {
        return false;
    }
    if (*peek < begin || end <= *peek) {
        return false;
    }

    p->next();
    c = *peek;
    return true;
}

bool satify::parse(reader_ptr &p, char &c) const {
    auto peek = p->peek();
    if (!peek) {
        return false;
    }

    if (!pred(*peek)) {
        return false;
    }

    p->next();
    c = *peek;
    return true;
}

std::optional<int> base_number(char c) {
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

bool digit_base::parse(reader_ptr &p, char &c) const {
    auto peek = p->peek();
    if (!peek) {
        return false;
    }

    bool condition = false;
    if (base < 10) {
        condition = '0' <= *peek && *peek < '0' + base;
    } else {
        condition |= '0' <= *peek && *peek <= '9';
        condition |= 'a' <= *peek && *peek < 'a' + base - 10;
        condition |= 'A' <= *peek && *peek < 'A' + base - 10;
    }

    if (!condition) {
        return false;
    }

    c = *peek;
    p->next();
    return true;
}

} // namespace token

namespace tokens {

bool serial::parse(reader_ptr &p, std::string &s) const {

    for (char c : cmp) {
        auto peek = p->peek();
        if (!peek) {
            return false;
        }
        if (*peek != c) {
            return false;
        }
        p->next();
        s.push_back(*peek);
    }

    return true;
}

bool integer_parse(reader_ptr &p, std::string &s) {
    char c;
    // (+-)?
    if (token::sign.parse(p, c)) {
        s.push_back(c);
    }

    // (0[bodx])?
    if (token::signle('0').parse(p, c)) {
        s.push_back(c);
        if (token::base.parse(p, c)) {
            s.push_back(c);
            const int base = token::base_number(c).value();
            return tokens::digits1_base(base).parse(p, s);
        } else {
            return tokens::digits0.parse(p, s);
        }
    }
    // [0-9]+
    return tokens::digits0.parse(p, s);
}

bool real_parse(reader_ptr &p, std::string &s) {

    char c;
    // (+-)?
    if (token::sign.parse(p, c)) {
        s.push_back(c);
    }

    // 0[bodx]
    int base = 10;
    if (token::signle('0').parse(p, c)) {
        s.push_back(c);
        if (token::base.parse(p, c)) {
            s.push_back(c);
            base = token::base_number(c).value();
            if (!tokens::digits0_base(base).parse(p, s)) {
                return false;
            }
        } else {
            if (!tokens::digits0.parse(p, s)) {
                return false;
            }
        }
    } else {
        // [0-9]*
        if (!tokens::digits0.parse(p, s)) {
            return false;
        }
    }

    // .
    if (!token::signle('.').parse(p, c)) {
        return false;
    }
    s.push_back(c);

    // digit
    if (!tokens::digits1_base(base).parse(p, s)) {
        return false;
    }

    // (e(+-)?[0-9]+)?
    if (token::list({'e', 'E'}).parse(p, c)) {
        s.push_back(c);
        if (token::sign.parse(p, c)) {
            s.push_back(c);
        }
        tokens::digits1.parse(p, s);
    }
    return true;
}

bool variable_parse(reader_ptr &p, std::string &s) {
    char c;
    if (!token::alpha.parse(p, c)) {
        return false;
    }
    s.push_back(c);
    while (token::alnum.parse(p, c)) {
        s.push_back(c);
    }
    return true;
}

bool comment_parse(reader_ptr &reader, std::string &s) {
    char c;
    bool ret = false;
    do {
        // line comment
        if (tokens::attempt(tokens::serial("//")).parse(reader, s)) {
            ret |= true;
            do {
                if (!token::any.parse(reader, c)) {
                    return ret;
                }
                s.push_back(c);
            } while (c != '\n' && c != '\r');
            continue;
        }

        // block comment
        if (tokens::attempt(tokens::serial("/*")).parse(reader, s)) {
            do {
                if (tokens::attempt(tokens::serial("*/")).parse(reader, s)) {
                    ret |= true;
                    break;
                }
                if (!token::any.parse(reader, c)){
                    return false;//対応するペアが見つからない
                }
                s.push_back(c);
            }while(1);
        }
        return ret;
    } while (1);
}

} // namespace tokens

} // namespace parsers