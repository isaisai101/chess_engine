#include <iostream>
#include "core/Board.h"

int main() {
    Board b;
    b.load_fen(START_FEN);
    b.print();
    std::cout << b.generate_legal_moves().count << '\n';   // should be 20
}