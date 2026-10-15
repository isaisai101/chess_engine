#pragma once
#include "type.h"
#include <utility>
#include <string>
#include <sstream>
#include <cctype>
#include <iostream>

class Board { 
public:
    Piece squares[64] = {};
    Side us = SIDE_WHITE;
    Side them = SIDE_BLACK;

    int king_sq[2] = {};
    int en_passant = -1;
    int castling = 0;

    int halfmove = 0;
    int fullmove = 1;

private:
    void add_pawn_move(int from, int to, MoveList& list) const {
        if (rank_of(to) == END_RANK[us]) {
            for (PieceType pt : PROMO_TYPES)
                list.add({from, to, NORMAL, pt});
        } else {
            list.add({from, to});
        }
    }

    void gen_pawn(int sq, MoveList& list) const {
        int f = file_of(sq), r = rank_of(sq);

        int to_f = f, to_r = r + PAWN_DIR[us].dr;
        if (in_bounds(to_f, to_r) && squares[square_of(to_f, to_r)] == EMPTY) {
            add_pawn_move(square_of(f, r), square_of(to_f, to_r), list);

            to_r += PAWN_DIR[us].dr;
            if (r == PAWN_START_RANK[us] && squares[square_of(to_f, to_r)] == EMPTY)
                list.add({square_of(f, r), square_of(to_f, to_r), DOUBLE_PUSH});
        }

        for (int i = 0; i < 2; i++) {
            to_f = f + PAWN_CAPTURES[us][i].df, to_r = r + PAWN_CAPTURES[us][i].dr;
            if (in_bounds(to_f, to_r)) {
                if (color_of(squares[square_of(to_f, to_r)]) == them)
                    add_pawn_move(square_of(f, r), square_of(to_f, to_r), list);
                else if (en_passant == square_of(to_f, to_r))
                    list.add({square_of(f, r), square_of(to_f, to_r), EN_PASSANT});
            }
        }
    }

    void gen_knight(int sq, MoveList& list) const {
        int f = file_of(sq), r = rank_of(sq);
        for (int i = 0; i < 8; i++) {
            int to_f = f + KNIGHT_DIRS[i].df, to_r = r + KNIGHT_DIRS[i].dr;
            if (in_bounds(to_f, to_r) && color_of(squares[square_of(to_f, to_r)]) != us)
                list.add({square_of(f, r), square_of(to_f, to_r)});
        }
    }

    void gen_slider(int sq, PieceType tp, MoveList& list) const {
        int f = file_of(sq), r = rank_of(sq);

        if (tp == ROOK || tp == QUEEN) {
            for (int i = 0; i < 4; i++) {
                int to_f = f + ROOK_DIRS[i].df, to_r = r + ROOK_DIRS[i].dr;
                while (in_bounds(to_f, to_r) && color_of(squares[square_of(to_f, to_r)]) != us) {
                    list.add({square_of(f, r), square_of(to_f, to_r)});
                    if (color_of(squares[square_of(to_f, to_r)]) == them)
                        break;
                    to_f += ROOK_DIRS[i].df, to_r += ROOK_DIRS[i].dr;
                }
            }
        }

        if (tp == BISHOP || tp == QUEEN) {
            for (int i = 0; i < 4; i++) {
                int to_f = f + BISHOP_DIRS[i].df, to_r = r + BISHOP_DIRS[i].dr;
                while (in_bounds(to_f, to_r) && color_of(squares[square_of(to_f, to_r)]) != us) {
                    list.add({square_of(f, r), square_of(to_f, to_r)});
                    if (color_of(squares[square_of(to_f, to_r)]) == them)
                        break;
                    to_f += BISHOP_DIRS[i].df, to_r += BISHOP_DIRS[i].dr;
                }
            }
        }
    }

    void gen_king(int sq, MoveList& list) const {
        int f = file_of(sq), r = rank_of(sq);
        for (int i = 0; i < 8; i++) {
            int to_f = f + KING_DIRS[i].df, to_r = r + KING_DIRS[i].dr;
            if (in_bounds(to_f, to_r) && color_of(squares[square_of(to_f, to_r)]) != us)
                list.add({square_of(f, r), square_of(to_f, to_r)});
        }
    }

    void gen_castling(MoveList& list) const {
        int base = START_RANK[us] * 8;
        
        if ((castling & CASTLING_RIGHT_KING[us]) 
            && squares[base+5] == EMPTY && squares[base+6] == EMPTY
            && !is_square_attacked(base + 4, them)
            && !is_square_attacked(base + 5, them)
            && !is_square_attacked(base + 6, them)) list.add({base + 4, base + 6, CASTLE});

        if ((castling & CASTLING_RIGHT_QUEEN[us]) 
            && squares[base+1] == EMPTY && squares[base+2] == EMPTY && squares[base+3] == EMPTY
            && !is_square_attacked(base + 4, them)
            && !is_square_attacked(base + 3, them)
            && !is_square_attacked(base + 2, them)) list.add({base + 4, base + 2, CASTLE});
    }

    MoveList generate_pseudo_moves() const {
        MoveList list;
        for (int sq = 0; sq < 64; sq++) {
            Piece p = squares[sq];
            if (p == EMPTY || color_of(p) != us) continue;

            switch (type_of(p)) {
                case PAWN:   gen_pawn(sq, list);   break;
                case KNIGHT: gen_knight(sq, list); break;
                case BISHOP: gen_slider(sq, BISHOP, list); break;
                case ROOK:   gen_slider(sq, ROOK, list);   break;
                case QUEEN:  gen_slider(sq, QUEEN, list);  break;
                case KING:   gen_king(sq, list);   break;
                default: break;
            }
        }
        gen_castling(list);

        return list;
    }

public:
    bool is_square_attacked(int sq, Side col) const {
        int f = file_of(sq), r = rank_of(sq);

        for (int i = 0; i < 2; i++) {
            int from_f = f - PAWN_CAPTURES[col][i].df, from_r = r - PAWN_CAPTURES[col][i].dr;
            if (in_bounds(from_f, from_r)) {
                Piece p = squares[square_of(from_f, from_r)];
                if (color_of(p) == col && type_of(p) == PAWN)
                    return true;
            }
        }

        for (int i = 0; i < 8; i++) {
            int from_f = f - KNIGHT_DIRS[i].df, from_r = r - KNIGHT_DIRS[i].dr;
            if (in_bounds(from_f, from_r)) {
                Piece p = squares[square_of(from_f, from_r)];
                if (color_of(p) == col && type_of(p) == KNIGHT)
                    return true;
            }
        }

        for (int i = 0; i < 4; i++) {
            int from_f = f - BISHOP_DIRS[i].df, from_r = r - BISHOP_DIRS[i].dr;
            while (in_bounds(from_f, from_r) && color_of(squares[square_of(from_f, from_r)]) != opposite(col)) {
                Piece p = squares[square_of(from_f, from_r)];
                if (color_of(p) == col) {
                    if (type_of(p) == BISHOP || type_of(p) == QUEEN)
                        return true;
                    break;
                }
                from_f -= BISHOP_DIRS[i].df, from_r -= BISHOP_DIRS[i].dr;
            }
        }

        for (int i = 0; i < 4; i++) {
            int from_f = f - ROOK_DIRS[i].df, from_r = r - ROOK_DIRS[i].dr;
            while (in_bounds(from_f, from_r) && color_of(squares[square_of(from_f, from_r)]) != opposite(col)) {
                Piece p = squares[square_of(from_f, from_r)];
                if (color_of(p) == col) {
                    if (type_of(p) == ROOK || type_of(p) == QUEEN)
                        return true;
                    break;
                }
                from_f -= ROOK_DIRS[i].df, from_r -= ROOK_DIRS[i].dr;
            }
        }

        for (int i = 0; i < 8; i++) {
            int from_f = f - KING_DIRS[i].df, from_r = r - KING_DIRS[i].dr;
            if (in_bounds(from_f, from_r)) {
                Piece p = squares[square_of(from_f, from_r)];
                if (color_of(p) == col && type_of(p) == KING)
                    return true;
            }
        }

        return false;
    }

    bool is_in_check(Side col) const {
        return is_square_attacked(king_sq[col], opposite(col));
    }

    void move(Move m) {
        bool capture = squares[m.to] != EMPTY || m.flag == EN_PASSANT;
        bool pawn_move = type_of(squares[m.from]) == PAWN;
        halfmove = (capture || pawn_move) ? 0 : halfmove + 1;
        if (us == SIDE_BLACK) fullmove++;

        castling &= CASTLE_MASK[m.from] & CASTLE_MASK[m.to];

        if (type_of(squares[m.from]) == KING) king_sq[us] = m.to;

        if (m.flag == EN_PASSANT) squares[square_of(file_of(m.to), rank_of(m.from))] = EMPTY;

        if (m.flag == DOUBLE_PUSH) en_passant = (m.from + m.to) / 2;
        else en_passant = -1;

        squares[m.to] = squares[m.from];
        squares[m.from] = EMPTY;

        if (m.promo != NO_TYPE) squares[m.to] = make_piece(us, m.promo);

        if (m.flag == CASTLE) {
            int base = rank_of(m.from) * 8;
            if (m.to == base + 6) std::swap(squares[base + 7], squares[base + 5]);   // h → f
            if (m.to == base + 2) std::swap(squares[base + 0], squares[base + 3]);   // a → d
        }

        us = opposite(us);
        them = opposite(them);
    }

    MoveList generate_legal_moves() const {
        MoveList list;

        MoveList pseudo = generate_pseudo_moves();

        for (int i = 0; i < pseudo.count; i++) {
            Board b = *this;
            b.move(pseudo.moves[i]);
            if (!b.is_in_check(us))
                list.add(pseudo.moves[i]);
        }

        return list;
    }

    void load_fen(const std::string& fen) {
        *this = Board{};   // clear everything to defaults

        std::istringstream ss(fen);
        std::string placement, side, castle, ep;
        ss >> placement >> side >> castle >> ep >> halfmove >> fullmove;

        // pieces: rank 8 first, a → h
        int f = 0, r = 7;
        for (char c : placement) {
            if (c == '/') {
                r--;
                f = 0;
            } else if (std::isdigit(c)) {
                f += c - '0';
            } else {
                Side s = std::isupper(c) ? SIDE_WHITE : SIDE_BLACK;
                char lower = static_cast<char>(std::tolower(c));
                PieceType t = static_cast<PieceType>(std::string("pnbrqk").find(lower) + 1);

                int sq = square_of(f, r);
                squares[sq] = make_piece(s, t);
                if (t == KING) king_sq[s] = sq;
                f++;
            }
        }

        // side to move
        us = (side == "w") ? SIDE_WHITE : SIDE_BLACK;
        them = opposite(us);

        // castling rights
        for (char c : castle) {
            if (c == 'K') castling |= WK;
            if (c == 'Q') castling |= WQ;
            if (c == 'k') castling |= BK;
            if (c == 'q') castling |= BQ;
        }

        // en passant square
        if (ep.size() == 2)
            en_passant = square_of(ep[0] - 'a', ep[1] - '1');
    }

    void print() const {
        for (int r = 7; r >= 0; r--) {
            std::cout << r + 1 << "  ";
            for (int f = 0; f < 8; f++)
                std::cout << ".PNBRQK??pnbrqk"[squares[square_of(f, r)]] << ' ';
            std::cout << '\n';
        }
        std::cout << "\n   a b c d e f g h\n";
        std::cout << (us == SIDE_WHITE ? "white" : "black") << " to move\n";
    }
};