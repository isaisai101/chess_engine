#pragma once

enum PieceType { NO_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

enum Piece {
    EMPTY = 0,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
};

enum Side { NO_SIDE = -1, SIDE_WHITE, SIDE_BLACK };

enum MoveFlag { NORMAL, DOUBLE_PUSH, EN_PASSANT, CASTLE };

enum CastleRight { WK = 1, WQ = 2, BK = 4, BQ = 8 };   // white/black, king-side/queen-side

constexpr int CASTLING_RIGHT_KING[2] = {WK, BK};
constexpr int CASTLING_RIGHT_QUEEN[2] = {WQ, BQ};

// castling &= CASTLE_MASK[from] & CASTLE_MASK[to]
// 15 = 1111 (keep all). Only the king and rook start squares remove rights:
//   a1: 13 (-WQ)   e1: 12 (-WK -WQ)   h1: 14 (-WK)
//   a8:  7 (-BQ)   e8:  3 (-BK -BQ)   h8: 11 (-BK)
inline constexpr int CASTLE_MASK[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,   // rank 1 (a1..h1)
    15, 15, 15, 15, 15, 15, 15, 15,   // rank 2
    15, 15, 15, 15, 15, 15, 15, 15,   // rank 3
    15, 15, 15, 15, 15, 15, 15, 15,   // rank 4
    15, 15, 15, 15, 15, 15, 15, 15,   // rank 5
    15, 15, 15, 15, 15, 15, 15, 15,   // rank 6
    15, 15, 15, 15, 15, 15, 15, 15,   // rank 7
     7, 15, 15, 15,  3, 15, 15, 11    // rank 8 (a8..h8)
};

inline constexpr PieceType PROMO_TYPES[4] = {QUEEN, ROOK, BISHOP, KNIGHT};

struct Move {
    int from, to;
    MoveFlag flag = NORMAL;
    PieceType promo = NO_TYPE;

    bool operator==(const Move& o) const {
        return from == o.from && to == o.to && flag == o.flag && promo == o.promo;
    }
};

struct MoveList {
    Move moves[256];
    int count = 0;
    void add(Move m) {
        moves[count] = m;
        count++;
    }
};

inline constexpr const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

inline Side opposite(Side s) { return static_cast<Side>(s ^ 1); }

constexpr int START_RANK[2] = {0, 7};
constexpr int END_RANK[2] = {7, 0};

struct Dir {
    int df, dr;
};

constexpr int PAWN_START_RANK[2] = {1, 6};

inline constexpr Dir PAWN_DIR[2] = {
    {0, 1},    // WHITE: forward is rank +1
    {0, -1}     // BLACK: forward is rank -1
};

inline constexpr Dir PAWN_CAPTURES[2][2] = {
    { {-1, 1}, {1, 1} },    // WHITE: forward is rank +1
    { {-1,-1}, {1,-1} }     // BLACK: forward is rank -1
};

inline constexpr Dir KNIGHT_DIRS[8] = {
    {1,2}, {2,1}, {2,-1}, {1,-2},
    {-1,-2}, {-2,-1}, {-2,1}, {-1,2}
};

inline constexpr Dir BISHOP_DIRS[4] = {
    {1,1}, {1,-1}, {-1,1}, {-1,-1}
};

inline constexpr Dir ROOK_DIRS[4] = {
    {1,0}, {-1,0}, {0,1}, {0,-1}
};

inline constexpr Dir QUEEN_DIRS[8] = {
    {1,0}, {-1,0}, {0,1}, {0,-1},
    {1,1}, {1,-1}, {-1,1}, {-1,-1}
};

inline constexpr Dir KING_DIRS[8] = {
    {1,0}, {-1,0}, {0,1}, {0,-1},
    {1,1}, {1,-1}, {-1,1}, {-1,-1}
};

inline bool in_bounds(int f, int r) {
    return f >= 0 && f < 8 && r >= 0 && r < 8;
}

inline Side color_of(Piece p) {
    return static_cast<Side>(p == EMPTY ? NO_SIDE : (p >> 3));
}

inline PieceType type_of(Piece p) {
    return static_cast<PieceType>(p & 7);
}

inline int square_of(int f, int r) {
    return r * 8 + f;
}

inline int file_of(int sq) {
    return sq % 8;
}

inline int rank_of(int sq) {
    return sq / 8;
}

inline Piece make_piece(Side s, PieceType t) {
    return static_cast<Piece>(s * 8 + t);
}