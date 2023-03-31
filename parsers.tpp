#pragma once

namespace parsers {
namespace token {
template <token_parser R, token_parser L> bool sum<R, L>::parse(reader_ptr &reader, char &c) const {
    if (r.parse(reader, c)) {
        return true;
    }
    if (l.parse(reader, c)) {
        return true;
    }
    return false;
}

template <token_parser R, token_parser L> bool meet<R, L>::parse(reader_ptr &reader, char &c) const {    
    const position keep=reader->get_position();
    
    if (!r.parse(reader, c)) {
        return false;
    }
    reader->set_position(keep);
    if (!l.parse(reader, c)) {
        return false;
    }
    return true;
}

template <token_parser P> bool invert<P>::parse(reader_ptr &reader, char &c) const {
    
    const position keep=reader->get_position();
    if (p.parse(reader, c)) {
        reader->set_position(keep);
        return false;
    }

    auto peek = reader->peek();
    if (!peek) {
        return false;
    }
    c = *peek;
    reader->next();
    return true;
}

template <token_parser P> bool opt<P>::parse(reader_ptr &reader, std::optional<char> &oc) const {
    char c;
    const bool result = parser.parse(reader, c);
    oc = result ? std::make_optional<char>(c) : std::nullopt;
    return true;
}

} // namespace token

namespace tokens {
template <token_parser P> bool from_token<P>::parse(reader_ptr &reader, std::string &s) const {
    char c;
    if (!parser.parse(reader, c)) {
        return false;
    }
    s.push_back(c);
    return true;
}
template <token_opt_parser P> bool from_token_opt<P>::parse(reader_ptr &reader, std::string &s) const {
    std::optional<char> oc;
    parser.parse(reader, oc);
    if (oc) {
        s.push_back(*oc);
    }
    return true;
}

template <tokens_parser P> bool many0<P>::parse(reader_ptr &p, std::string &s) const {
    while (parser.parse(p, s)) {
    }
    return true;
}

template <tokens_parser P> bool many1<P>::parse(reader_ptr &p, std::string &s) const {
    if (!parser.parse(p, s)) {
        return false;
    }

    while (parser.parse(p, s)) {
    }
    return true;
}

template <tokens_parser R, tokens_parser L> bool chain2<R, L>::parse(reader_ptr &p, std::string &s) const {
    if (!r.parse(p, s)) {
        return false;
    }

    if (!l.parse(p, s)) {
        return false;
    }

    return true;
}

template <tokens_parser P> bool attempt<P>::parse(reader_ptr &reader, std::string &s) const {
    // store
    const auto keep= reader->get_position();
    std::string s_backup = s;
    // string
    if (parser.parse(reader, s)) {
        return true;
    }
    // restore
    reader->set_position(keep);
    s = s_backup;
    return false;
}

} // namespace tokens
} // namespace parsers
