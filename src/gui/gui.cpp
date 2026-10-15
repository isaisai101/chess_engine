#include <string>
#include <vector>
#include "raylib.h"
#include "core/Board.h"
#include "ai/search.h"

// ---------- layout ----------
const int SQ = 80;              // size of one square in pixels
const int BOARD_PX = SQ * 8;    // board is 640x640
const int BAR = 40;             // status bar under the board

// ---------- AI ----------
const int AI_TIME_MS = 3000;    // thinking time per move (the search goes as deep as it can)

// ---------- colors ----------
const Color LIGHT_SQ  = {240, 217, 181, 255};
const Color DARK_SQ   = {181, 136,  99, 255};
const Color LAST_MOVE = {205, 210, 106, 160};
const Color CHECK_SQ  = {230,  70,  70, 200};
const Color HINT      = { 40,  40,  40,  90};
const Color LABEL     = { 90,  60,  40, 255};

Texture2D sheet;

bool flipped = false;           // board seen from black's side

// top-left pixel of a square on screen
Vector2 square_pos(int sq) {
    int col = flipped ? 7 - file_of(sq) : file_of(sq);
    int row = flipped ? rank_of(sq) : 7 - rank_of(sq);
    return { (float)(col * SQ), (float)(row * SQ) };
}

// square under a screen point, or -1 if outside the board
int square_at(Vector2 p) {
    if (p.x < 0 || p.y < 0 || p.x >= BOARD_PX || p.y >= BOARD_PX) return -1;
    int col = (int)p.x / SQ, row = (int)p.y / SQ;
    int f = flipped ? 7 - col : col;
    int r = flipped ? row : 7 - row;
    return square_of(f, r);
}

void draw_piece(Piece p, float cx, float cy) {
    // column in the sheet for each PieceType (sheet order: K Q B N R P)
    const int COL[7] = {0, 5, 3, 2, 4, 1, 0};   // NO_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING

    if (p == EMPTY) return;
    float cw = sheet.width / 6.0f, ch = sheet.height / 2.0f;
    Rectangle src = { COL[type_of(p)] * cw, (color_of(p) == SIDE_WHITE ? 0 : 1) * ch, cw, ch };
    Rectangle dst = { cx - SQ / 2.0f, cy - SQ / 2.0f, (float)SQ, (float)SQ };
    DrawTexturePro(sheet, src, dst, {0, 0}, 0, WHITE);
}

// find the legal move from -> to (auto-queen on promotion)
bool find_move(const MoveList& legal, int from, int to, Move& out) {
    for (int i = 0; i < legal.count; i++) {
        const Move& m = legal.moves[i];
        if (m.from == from && m.to == to && (m.promo == NO_TYPE || m.promo == QUEEN)) {
            out = m;
            return true;
        }
    }
    return false;
}

int main() {
    InitWindow(BOARD_PX, BOARD_PX + BAR, "Chess  -  U undo   R reset   F flip   S switch side");
    SetTargetFPS(60);

    sheet = LoadTexture("assets/pieces.png");
    SetTextureFilter(sheet, TEXTURE_FILTER_BILINEAR);

    Board board;
    std::vector<Board> history;                 // for undo
    MoveList legal;

    bool dragging = false;
    int drag_from = -1;
    int last_from = -1, last_to = -1;

    // ---------- AI state ----------
    Search ai;
    Side human = SIDE_WHITE;
    bool ai_ready = false;      // false: draw one "thinking" frame first, true: search now
    int  ai_depth = 0;          // depth the last AI search finished (0 = no search yet)

    // start a new game, you get a random side
    // (InitWindow already seeded raylib's random numbers with the time)
    auto new_game = [&]() {
        board.load_fen(START_FEN);
        history.clear();
        legal = board.generate_legal_moves();
        last_from = last_to = -1;
        dragging = false;
        ai_ready = false;
        ai_depth = 0;
        human = GetRandomValue(0, 1) == 0 ? SIDE_WHITE : SIDE_BLACK;
        flipped = (human == SIDE_BLACK);        // your pieces at the bottom
    };
    new_game();

    while (!WindowShouldClose()) {
        // ================= input =================
        Vector2 mouse = GetMousePosition();

        // pick up a piece (only on your turn, and only while the game is on)
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && board.us == human && legal.count > 0) {
            int sq = square_at(mouse);
            if (sq != -1 && color_of(board.squares[sq]) == board.us) {
                dragging = true;
                drag_from = sq;
            }
        }

        // drop it: play the move only if it's in the legal list
        if (dragging && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            int to = square_at(mouse);
            Move m;
            if (to != -1 && find_move(legal, drag_from, to, m)) {
                history.push_back(board);
                board.move(m);
                legal = board.generate_legal_moves();
                last_from = m.from;
                last_to = m.to;
            }
            dragging = false;
            drag_from = -1;
        }

        // U = undo back to your last turn
        if (IsKeyPressed(KEY_U) && !history.empty()) {
            do {
                board = history.back();
                history.pop_back();
            } while (board.us != human && !history.empty());
            legal = board.generate_legal_moves();
            last_from = last_to = -1;
            dragging = false;
            ai_ready = false;
        }

        // R = new game (new random side)
        if (IsKeyPressed(KEY_R)) new_game();

        // F = flip view, S = play the other color
        if (IsKeyPressed(KEY_F)) flipped = !flipped;
        if (IsKeyPressed(KEY_S)) {
            human = opposite(human);
            flipped = (human == SIDE_BLACK);
            dragging = false;
            ai_ready = false;
        }

        // ================= AI move =================
        // The search blocks the window. So when it becomes the AI's turn, we first
        // draw one frame (your move + "thinking..."), and search on the next frame.
        if (board.us != human && legal.count > 0) {
            if (!ai_ready) {
                ai_ready = true;
            } else {
                SearchResult r = ai.think(board, AI_TIME_MS);
                Move m = r.move;
                ai_depth = r.depth;

                history.push_back(board);
                board.move(m);
                legal = board.generate_legal_moves();
                last_from = m.from;
                last_to = m.to;
                ai_ready = false;
            }
        }

        // ================= draw =================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        bool in_check = board.is_in_check(board.us);

        // squares + last move + check
        for (int sq = 0; sq < 64; sq++) {
            Vector2 p = square_pos(sq);
            bool light = (file_of(sq) + rank_of(sq)) % 2 == 1;   // a1 is dark
            DrawRectangle((int)p.x, (int)p.y, SQ, SQ, light ? LIGHT_SQ : DARK_SQ);
            if (sq == last_from || sq == last_to)
                DrawRectangle((int)p.x, (int)p.y, SQ, SQ, LAST_MOVE);
            if (in_check && sq == board.king_sq[board.us])
                DrawRectangle((int)p.x, (int)p.y, SQ, SQ, CHECK_SQ);
        }

        // file and rank labels
        for (int i = 0; i < 8; i++) {
            int f = flipped ? 7 - i : i;
            int r = flipped ? i : 7 - i;
            char fl[2] = { (char)('a' + f), '\0' };
            char rl[2] = { (char)('1' + r), '\0' };
            DrawText(fl, i * SQ + SQ - 12, BOARD_PX - 18, 14, LABEL);
            DrawText(rl, 4, i * SQ + 4, 14, LABEL);
        }

        // pieces (skip the one being dragged)
        for (int sq = 0; sq < 64; sq++) {
            if (dragging && sq == drag_from) continue;
            Vector2 p = square_pos(sq);
            draw_piece(board.squares[sq], p.x + SQ / 2, p.y + SQ / 2);
        }

        // legal targets of the dragged piece
        if (dragging) {
            for (int i = 0; i < legal.count; i++) {
                const Move& m = legal.moves[i];
                if (m.from != drag_from) continue;
                Vector2 p = square_pos(m.to);
                if (board.squares[m.to] != EMPTY)
                    DrawRectangleLinesEx({ p.x, p.y, (float)SQ, (float)SQ }, 4, HINT);
                else
                    DrawCircle((int)p.x + SQ / 2, (int)p.y + SQ / 2, SQ * 0.15f, HINT);
            }
            draw_piece(board.squares[drag_from], mouse.x, mouse.y);
        }

        // game over: dark layer over the board + big result text
        if (legal.count == 0) {
            const char* title;
            const char* reason;
            Color col;
            if (!in_check)              { title = "DRAW";     reason = "stalemate"; col = LIGHTGRAY; }
            else if (board.us == human) { title = "YOU LOSE"; reason = "checkmate"; col = Color{230,  80,  80, 255}; }
            else                        { title = "YOU WIN!"; reason = "checkmate"; col = Color{110, 210, 110, 255}; }

            DrawRectangle(0, 0, BOARD_PX, BOARD_PX, Color{0, 0, 0, 150});

            int tw = MeasureText(title, 80);
            DrawText(title, (BOARD_PX - tw) / 2, BOARD_PX / 2 - 60, 80, col);

            const char* hint = TextFormat("%s  -  press R for a new game", reason);
            int hw = MeasureText(hint, 20);
            DrawText(hint, (BOARD_PX - hw) / 2, BOARD_PX / 2 + 35, 20, RAYWHITE);
        }

        // status bar
        DrawRectangle(0, BOARD_PX, BOARD_PX, BAR, Color{30, 30, 30, 255});
        std::string status;
        if (legal.count == 0) {
            if (!in_check)               status = "Stalemate";
            else if (board.us == human)  status = "Checkmate - AI wins";
            else                         status = "Checkmate - you win!";
        } else if (board.us != human) {
            status = "AI thinking...";
        } else {
            status = std::string("Your move") + (in_check ? " - check!" : "");
        }
        DrawText(status.c_str(), 10, BOARD_PX + 10, 20, RAYWHITE);

        const char* info = ai_depth > 0
            ? TextFormat("You: %s   AI %.1fs, depth %d",
                         human == SIDE_WHITE ? "White" : "Black", AI_TIME_MS / 1000.0, ai_depth)
            : TextFormat("You: %s   AI %.1fs",
                         human == SIDE_WHITE ? "White" : "Black", AI_TIME_MS / 1000.0);
        DrawText(info, BOARD_PX - MeasureText(info, 16) - 10, BOARD_PX + 12, 16, GRAY);

        EndDrawing();
    }

    UnloadTexture(sheet);
    CloseWindow();
}