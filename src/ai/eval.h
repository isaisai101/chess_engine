#pragma once
#include "core/board.h"

inline constexpr int INF  = 1'000'000;
inline constexpr int MATE =   100'000;

inline constexpr int PIECE_VALUE[7] = { 0, 100, 320, 330, 500, 900, MATE };

inline int evaluate(const Board& b) {
    MoveList list = b.generate_legal_moves();
    if (list.count == 0)
        return b.is_in_check(b.us) ? -(MATE) : 0;
    int res = 0;
    for (int i = 0; i < 64; i++) {
        if (b.squares[i] == EMPTY) continue;
        Piece p = b.squares[i];
        res += PIECE_VALUE[type_of(p)] * (color_of(p) == b.us ? 1 : -1);
    }

    return res;
}
