// uci.cpp — the UCI loop. Lets GUIs (Cute Chess, Arena) and match runners
// (fastchess, cutechess-cli) talk to the engine through stdin/stdout.
//
// RULE: nothing else in the program may print to stdout (std::cout / printf),
// or the GUI will read it as a command. Debug output goes to std::cerr.

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include "core/board.h"
#include "ai/search.h"

namespace {

constexpr const char* ENGINE_NAME   = "is_i";
constexpr const char* ENGINE_AUTHOR = "Isatat";
constexpr int MAX_DEPTH       = 64;              // depth cap when "go" gives none
constexpr int DEFAULT_TIME_MS = 1000;            // plain "go" with no depth or clock
constexpr int NO_TIME_LIMIT   = 1'000'000'000;   // "go depth N": let the depth decide

// ---------- move text <-> Move ----------

std::string square_name(int sq) {          // assumes square = rank * 8 + file, a1 = 0
    std::string s;
    s += char('a' + sq % 8);
    s += char('1' + sq / 8);
    return s;
}

char promo_char(PieceType t) {
    switch (t) {
        case KNIGHT: return 'n';
        case BISHOP: return 'b';
        case ROOK:   return 'r';
        case QUEEN:  return 'q';
        default:     return 0;              // no promotion
    }
}

std::string move_to_uci(const Move& m) {    // e2e4, e7e8q, e1g1 (castling = king move)
    std::string s = square_name(m.from) + square_name(m.to);
    if (char c = promo_char(m.promo)) s += c;
    return s;
}

// Find the legal move that matches the text. The flags (castle, en passant,
// double push) come from movegen, so they never have to be guessed here.
bool parse_move(const Board& b, const std::string& text, Move& out) {
    if (text.size() < 4) return false;
    int from   = square_of(text[0] - 'a', text[1] - '1');
    int to     = square_of(text[2] - 'a', text[3] - '1');
    char promo = text.size() > 4 ? text[4] : 0;

    MoveList list = b.generate_legal_moves();
    for (int i = 0; i < list.count; i++) {
        const Move& m = list.moves[i];
        if (m.from == from && m.to == to && promo_char(m.promo) == promo) {
            out = m;
            return true;
        }
    }
    return false;
}

// ---------- score text ----------

// Your negamax scores a mate as MATE + (remaining depth), so the number of
// plies from the root to the mate can be worked back from the root depth.
// If you change how mates are scored (e.g. MATE - ply), update this.
std::string score_to_uci(int score, int depth) {
    if (std::abs(score) < MATE)
        return "cp " + std::to_string(score);
    int ply   = depth - (std::abs(score) - MATE);
    int moves = score > 0 ? (ply + 1) / 2 : -(ply / 2);
    return "mate " + std::to_string(moves);
}

// ---------- commands ----------

// position startpos [moves e2e4 e7e5 ...]
// position fen <6 fields> [moves ...]
void handle_position(Board& board, std::istringstream& ss) {
    std::string token, fen;
    ss >> token;

    if (token == "startpos") {
        fen = START_FEN;
        ss >> token;                        // "moves" or nothing
    } else if (token == "fen") {
        while (ss >> token && token != "moves")
            fen += token + " ";
    } else {
        return;
    }

    board.load_fen(fen);
    if (token != "moves") return;

    while (ss >> token) {
        Move m;
        if (!parse_move(board, token, m)) {
            std::cout << "info string illegal move " << token << std::endl;
            return;
        }
        board.move(m);
    }
}

// go [depth N] [movetime ms] [wtime ms] [btime ms] [winc ms] [binc ms]
//   go depth 6              -> search to depth 6, no time limit
//   go movetime 2000        -> think exactly 2 seconds
//   go wtime .. btime ..    -> pick a time from the clock
//   go (nothing)            -> DEFAULT_TIME_MS
// "go infinite" and "stop" need the search on its own thread, so they're not supported yet.
void handle_go(const Board& board, Search& search, std::istringstream& ss) {
    int depth = MAX_DEPTH;
    int movetime = -1, wtime = -1, btime = -1, winc = 0, binc = 0;

    std::string token;
    while (ss >> token) {
        if      (token == "depth")    ss >> depth;
        else if (token == "movetime") ss >> movetime;
        else if (token == "wtime")    ss >> wtime;
        else if (token == "btime")    ss >> btime;
        else if (token == "winc")     ss >> winc;
        else if (token == "binc")     ss >> binc;
    }

    int my_time = board.us == SIDE_WHITE ? wtime : btime;
    int my_inc  = board.us == SIDE_WHITE ? winc  : binc;

    int time_ms;
    if (movetime > 0)          time_ms = movetime;
    else if (my_time > 0)      time_ms = my_time / 30 + my_inc / 2;
    else if (depth < MAX_DEPTH) time_ms = NO_TIME_LIMIT;   // only a depth was given
    else                       time_ms = DEFAULT_TIME_MS;

    if (board.generate_legal_moves().count == 0) {         // mate or stalemate
        std::cout << "bestmove 0000" << std::endl;
        return;
    }

    SearchResult r = search.think(board, time_ms, depth);
    if (r.depth > 0)                                       // 0 = time ran out in depth 1, no real score
        std::cout << "info depth " << r.depth
                  << " score " << score_to_uci(r.score, r.depth) << std::endl;
    std::cout << "bestmove " << move_to_uci(r.move) << std::endl;
}

} // namespace

int main() {
    Board board;
    board.load_fen(START_FEN);
    Search search;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "uci") {
            std::cout << "id name "   << ENGINE_NAME   << "\n"
                      << "id author " << ENGINE_AUTHOR << "\n"
                      << "uciok" << std::endl;
        } else if (cmd == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (cmd == "ucinewgame") {
            board.load_fen(START_FEN);
        } else if (cmd == "position") {
            handle_position(board, ss);
        } else if (cmd == "go") {
            handle_go(board, search, ss);
        } else if (cmd == "d") {               // not UCI, just for you: print the board
            board.print();
        } else if (cmd == "quit") {
            break;
        }
        // "stop", "setoption", "debug", "ponderhit": ignored for now
    }
    return 0;
}