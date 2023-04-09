#pragma once
namespace tokenize::parsers {

template <class T>
bool sum<T>::operator()(reader_ptr &reader, T &out) const {
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
