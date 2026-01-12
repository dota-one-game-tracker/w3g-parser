#include <baje_parser.h>
#include <iostream>

int main(int argc, char **argv) {
    if (argc != 1) {
        std::cout << argv[0] << " takes no arguments.\n";
        return 1;
    }
    baje_parser::Baje_parser c;
    return c.get_number() != 6;
}
