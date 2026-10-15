#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include "core/Board.h"

// count all positions reachable in exactly `depth` moves
uint64_t perft(const Board& b, int depth) {
    MoveList moves = b.generate_legal_moves();
    if (depth == 1) return moves.count;          // shortcut: no need to make the last moves

    uint64_t nodes = 0;
    for (int i = 0; i < moves.count; i++) {
        Board copy = b;
        copy.move(moves.moves[i]);
        nodes += perft(copy, depth - 1);
    }
    return nodes;
}

// "e2e4", "e7e8q"
std::string move_str(const Move& m) {
    std::string s;
    s += char('a' + file_of(m.from)); s += char('1' + rank_of(m.from));
    s += char('a' + file_of(m.to));   s += char('1' + rank_of(m.to));
    if (m.promo != NO_TYPE) s += " pnbrqk"[m.promo];
    return s;
}

// per-move counts, to compare with Stockfish's "go perft N"
void divide(const Board& b, int depth) {
    MoveList moves = b.generate_legal_moves();
    uint64_t total = 0;
    for (int i = 0; i < moves.count; i++) {
        Board copy = b;
        copy.move(moves.moves[i]);
        uint64_t n = depth == 1 ? 1 : perft(copy, depth - 1);
        std::cout << move_str(moves.moves[i]) << ": " << n << '\n';
        total += n;
    }
    std::cout << "total: " << total << "\n\n";
}

struct Test {
    const char* name;
    const char* fen;
    uint64_t expected[6];   // depth 1..6, 0 = skip
};

const Test TESTS[] = {
    { "start", START_FEN,
      { 20, 400, 8902, 197281, 4865609, 0 } },
    { "kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
      { 48, 2039, 97862, 4085603, 0, 0 } },
    { "position 3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
      { 14, 191, 2812, 43238, 674624, 0 } },
};

int main() {
    for (const Test& t : TESTS) {
        std::cout << "== " << t.name << " ==\n";
        Board b;
        b.load_fen(t.fen);

        for (int d = 1; d <= 6 && t.expected[d - 1] != 0; d++) {
            auto start = std::chrono::steady_clock::now();
            uint64_t n = perft(b, d);
            double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

            bool ok = n == t.expected[d - 1];
            std::cout << "depth " << d << ": " << n
                      << (ok ? "  ok" : "  FAIL (expected " + std::to_string(t.expected[d - 1]) + ")")
                      << "  [" << sec << "s]\n";
            if (!ok) break;     // deeper depths will be wrong too
        }
        std::cout << '\n';
    }

    // when something FAILs: uncomment, set the FEN and depth, compare with Stockfish
    // Board b; b.load_fen(START_FEN); divide(b, 3);
}