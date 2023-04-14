#pragma once
namespace tokenize::parsers {

template <class R, class L> bool chain<R, L>::operator()(reader_ptr &reader, std::string &s) const {

    if (!right(reader, s)) {
        return false;
    }

    return left(reader, s);
}

template <class T>
repeat_range<T>::repeat_range(const T &_parser, unsigned int _min, unsigned int _max)
    : parser(_parser), min(_min), max(_max) {
    assert(min <= max);
}

template <class T> bool repeat_range<T>::operator()(reader_ptr &reader, std::string &s) const {
    int count = 0;

    // min
    {
        const position pos = reader->get_position();
        for (; count < min; count++) {
            if (!parser(reader, s)) {
                if (pos != reader->get_position()) {
                    std::cerr << "overrun (repeat min)" << std::endl;
                }
                return false;
            }
        }
    }

    // ~max
    for (; count < max; count++) {
        const position pos = reader->get_position();
        if (!parser(reader, s)) {
            if (pos != reader->get_position()) {
                std::cerr << "overrun (repeat max)" << std::endl;
            }
            return true;
        }
    }
    return true;
}

template <class T> bool sum<T>::operator()(reader_ptr &reader, T &out) const {
    // store
    const auto keep = reader->get_position();

    if (right(reader, out)) {
        return true;
    }
    // error check
    if (keep != reader->get_position()) {
        std::cerr << "overrun" << std::endl;
        return false;
    }

    return left(reader, out);
}

template <class T> bool attempt<T>::operator()(reader_ptr &reader, T &out) const {
    // store
    const T out_keep = out;
    const position p_keep = reader->get_position();
    // parse
    if (parser(reader, out)) {
        return true;
    }
    // restore
    out = out_keep;
    reader->set_position(p_keep);
    return false;
}
} // namespace tokenize::parsers
