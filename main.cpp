#include "parsers.hpp"

#include <iostream>

int main(int argc, char **argv) {
    using namespace parsers;
    using namespace std;

    string line;
    while (std::getline(cin, line)) {
        std::string_view name = "stdin";
        reader_ptr r = readers::make_string_reader(name, line);
        std::string s;
        auto ret = (tokens::comment).parse(r, s);
        cout << line << "->" << (ret ? "true" : "false") << ":" << s << endl;

        // if (tokens::serial("++").parse(r, s)) {
        //     cout << "tokens:" << s << endl;
        // } else if (tokens::variable.parse(r, s)) {
        //     cout << "var:" << s << endl;
        // } else if (tokens::attempt(tokens::real).parse(r, s)) {
        //     cout << "real:" << s << endl;
        // } else if (tokens::integer.parse(r, s)) {
        //     cout << "int:" << s << endl;
        // } else {
        //     cout << "failed" << endl;
        // }
    }

    return 0;
}
