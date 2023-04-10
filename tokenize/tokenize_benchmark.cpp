#include "tokenize.hpp"
#include <chrono>
#include <iostream>

int main(int argc, char **argv) {
    using namespace std;
    using namespace tokenize;
    cout << "benchmark" << endl;

    int n = 1000;
    if (argc > 1) {
        n = atoi(argv[1]);
    }

    auto reader = make_string_reader("func main(){\n"
                                     "  int x=10+10;\n"
                                     "  return 0\n"
                                     "}");
    auto position = reader->get_position();
    std::vector<token> tokens;

    auto begin = std::chrono::system_clock::now();

    for (int i = 0; i < n; i++) {
        tokenize_all(reader, tokens);
        reader->set_position(position);
    }
    auto end = std::chrono::system_clock::now();
    double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    cout << "elapsed:" << elapsed / n << "ms" << endl;
    return 0;
}
