#pragma once
namespace tokenize::parsers {

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
