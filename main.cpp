#include "tokenize/tokenize.hpp"

#include <iostream>

int main(int argc, char **argv) {
    using namespace tokenize;
    using namespace std;

    string line;
    while (std::getline(cin, line)) {
        reader_ptr reader = tokenize::make_string_reader(line);
        std::string s = "";

        std::vector<token> ts;
        if (tokenize_all(reader, ts)) {
            cout << ts << endl;
        } else {
            cout << "failed" << endl;
        }
    }

    return 0;
}
