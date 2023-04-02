#include "parsers.hpp"

#include <iostream>

int main(int argc, char **argv) {
    using namespace parsers;
    using namespace std;

    string line;
    while (std::getline(cin, line)) {
        std::string_view name = "stdin";
        reader_ptr reader = readers::make_string_reader(name, line);
        std::string s = "";

        int x;
        token t;
        if (tokenize(reader, t)) {
            cout <<t<<endl;
        }else{
            cout<<"failed"<<endl;
        }
    }

    return 0;
}
