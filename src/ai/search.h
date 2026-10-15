#pragma once
#include "core/board.h"
#include "eval.h"
#include <algorithm>
#include <chrono>
#include <utility>

struct SearchResult {
    Move move{};
    int score = -INF;        // centipawns, from the side to move's point of view
    int depth = 0;
};

inline constexpr int CAPTURE_BONUS   = 100'000;
inline constexpr int PROMOTION_BONUS = 200'000;

// Higher score = try the move earlier.
// Captures use MVV-LVA: biggest victim first, then smallest attacker.
inline int move_score(const Board& b, const Move& m) {
    int score = 0;

    if (m.flag == EN_PASSANT) {
        score += CAPTURE_BONUS + PIECE_VALUE[PAWN] * 10 - PIECE_VALUE[PAWN];
    } else if (b.squares[m.to] != EMPTY) {
        PieceType victim   = type_of(b.squares[m.to]);
        PieceType attacker = type_of(b.squares[m.from]);
        score += CAPTURE_BONUS + PIECE_VALUE[victim] * 10 - PIECE_VALUE[attacker];
    }

    if (m.promo == QUEEN)                  // under-promotions stay low, they're rarely good
        score += PROMOTION_BONUS;

    return score;
}

class Search {
    using Clock = std::chrono::steady_clock;
    Clock::time_point start;
    int  time_limit_ms = 0;
    bool stopped = false;
    long long nodes = 0;

    int elapsed_ms() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
    }

    void order_moves(const Board& b, MoveList& list) {
        auto cmp = [&](const Move& x, const Move& y) {
            return move_score(b, x) > move_score(b, y);
        };
        std::sort(list.moves, list.moves + list.count, cmp);
    }

    bool time_up() {
        if ((++nodes & 2047) == 0 && elapsed_ms() >= time_limit_ms)
            stopped = true;
        return stopped;
    }

    // Only captures, until the position is quiet.
    int quiescence(const Board& b, int alpha, int beta) {
        if (time_up()) return 0;

        int stand_pat = evaluate(b);            // "I can just stop here"
        if (stand_pat >= beta)
            return stand_pat;
        alpha = std::max(alpha, stand_pat);

        MoveList list = b.generate_legal_moves();
        order_moves(b, list);

        for (int i = 0; i < list.count; i++) {
            if (move_score(b, list.moves[i]) < CAPTURE_BONUS)
                break;                          // sorted, so the rest are quiet moves

            Board copy = b;
            copy.move(list.moves[i]);
            int score = -quiescence(copy, -beta, -alpha);
            if (stopped) return 0;

            alpha = std::max(alpha, score);
            if (alpha >= beta)
                break;
        }
        return alpha;
    }
    int negamax(const Board& b, int alpha, int beta, int depth) {
        if (depth <= 0)
            return quiescence(b, alpha, beta);
        if (time_up()) return 0;

        MoveList list = b.generate_legal_moves();
        if (list.count == 0)
            return b.is_in_check(b.us) ? -(MATE + depth) : 0;
        order_moves(b, list);

        for (int i = 0; i < list.count; i++) {
            Board copy = b;
            copy.move(list.moves[i]);
            int score = -negamax(copy, -beta, -alpha, depth - 1);
            if (stopped) return 0;

            alpha = std::max(alpha, score);
            if (alpha >= beta)
                break;
        }
        return alpha;
    }
    SearchResult search_root(const Board& b, int depth, const Move& first) {
        MoveList list = b.generate_legal_moves();

        order_moves(b, list);
        for (int i = 0; i < list.count; i++) {
            if (list.moves[i] == first) {
                std::swap(list.moves[0], list.moves[i]);
                break;
            }
        }

        SearchResult best{};
        for (int i = 0; i < list.count; i++) {
            Board copy = b;
            copy.move(list.moves[i]);

            int res = negamax(copy, -INF, -best.score, depth - 1);
            if (best.score < -res) {
                best.move = list.moves[i];
                best.score = -res;
            }
        }

        return best;
    }

public:
    SearchResult think(const Board& b, int time_ms, int max_depth = 64) {
        start = Clock::now();
        time_limit_ms = time_ms;
        stopped = false;
        nodes = 0;

        SearchResult best{b.generate_legal_moves().moves[0], -INF};   // fallback

        for (int depth = 1; depth <= max_depth; depth++) {
            SearchResult r = search_root(b, depth, best.move);
            if (stopped) break;                       // unfinished: keep the old result
            best = r;
            best.depth = depth;

            if (std::abs(best.score) >= MATE) break;  // found a mate, no need to go deeper
            if (elapsed_ms() > time_ms / 2) break;    // next depth won't finish in time
        }
        return best;
    }
};