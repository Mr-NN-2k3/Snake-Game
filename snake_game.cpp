#include <bits/stdc++.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#endif

using namespace std;

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>

static struct termios orig_termios;

void resetTerminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void initTerminal() {
    struct termios newt;
    tcgetattr(STDIN_FILENO, &orig_termios);
    newt = orig_termios;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    atexit(resetTerminal);
}

bool _kbhit() {
    fd_set set;
    struct timeval tv = {0, 0};
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0;
}

int _getch() {
    unsigned char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) == 1)
        return ch;
    return -1;
}

void enableVTAndUTF8() {
    setlocale(LC_ALL, "en_US.UTF-8");
    initTerminal();
}
#endif

struct Vec { int r, c; };
enum Dir { UP=0, RIGHT=1, DOWN=2, LEFT=3 };
enum FoodType { NORMAL, GOLDEN, POISON };
enum PowerUpType { SLOW_MOTION, INVINCIBILITY, MAGNET, NONE };

namespace Color {
    const string RESET = "\x1b[0m", BOLD = "\x1b[1m", DIM = "\x1b[2m", UNDERLINE = "\x1b[4m";
    const string RED = "\x1b[31m", GREEN = "\x1b[32m", YELLOW = "\x1b[33m", BLUE = "\x1b[34m";
    const string MAGENTA = "\x1b[35m", CYAN = "\x1b[36m", WHITE = "\x1b[37m";
    const string BRIGHT_RED = "\x1b[91m", BRIGHT_GREEN = "\x1b[92m", BRIGHT_YELLOW = "\x1b[93m";
    const string BRIGHT_BLUE = "\x1b[94m", BRIGHT_MAGENTA = "\x1b[95m", BRIGHT_CYAN = "\x1b[96m";
    const string BRIGHT_WHITE = "\x1b[97m";
    string bg(int id) { return "\x1b[48;5;" + to_string(id) + "m"; }
    string fg(int id) { return "\x1b[38;5;" + to_string(id) + "m"; }
}

static const string SNAKE_HEAD = Color::BRIGHT_GREEN + "🐉" + Color::RESET;
static const string SNAKE_BODY = Color::GREEN + "🐉" + Color::RESET;
static const string SNAKE_INVINCIBLE = Color::BRIGHT_CYAN + "🐉" + Color::RESET;
static const string FOOD_NORMAL = Color::BRIGHT_RED + "🍎" + Color::RESET;
static const string FOOD_GOLDEN = Color::BRIGHT_YELLOW + "⭐" + Color::RESET;
static const string FOOD_POISON = Color::MAGENTA + "☠️" + Color::RESET;
static const string OBSTACLE = Color::fg(240) + "█" + Color::RESET;
static const string DYNAMIC_OBSTACLE = Color::BRIGHT_RED + "▓" + Color::RESET;
static const string EMPTY = Color::bg(235) + "  " + Color::RESET;
static const string WALL = Color::BRIGHT_CYAN + "█" + Color::RESET;
static const string POWERUP_SLOW = Color::BRIGHT_BLUE + "⏱" + Color::RESET;
static const string POWERUP_SHIELD = Color::BRIGHT_CYAN + "🛡" + Color::RESET;
static const string POWERUP_MAGNET = Color::BRIGHT_MAGENTA + "🧲" + Color::RESET;


struct KeyBindings {
    char up = 'w', down = 's', left = 'a', right = 'd', quit = 'q', pause = 'p';
};

struct PowerUp {
    Vec pos;
    PowerUpType type;
    int duration;
    bool active;
};

struct DynamicObstacle {
    Vec pos;
    Dir dir;
    int speed;
};

struct GameConfig {
    int rows = 20, cols = 30, baseFps = 8, currentFps = 8;
    int goldenAppleChance = 12, poisonAppleChance = 10;
    int normalAppleScore = 10, goldenAppleScore = 50, poisonApplePenalty = -30;
    bool bordersEnabled = true;
    int currentLevel = 1, maxLevel = 5;
    KeyBindings keys;
    bool debugMode = true;
};

struct GameState {
    deque<Vec> snake;
    Dir dir = RIGHT;
    Vec food{-1, -1};
    FoodType foodType = NORMAL;
    set<pair<int,int>> obstacles;
    vector<DynamicObstacle> dynamicObstacles;
    vector<PowerUp> powerUps;
    PowerUpType activePowerUp = NONE;
    int powerUpTimer = 0;
    bool gameOver = false, quit = false, paused = false;
    int score = 0, moves = 0;
    int goldenApplesEaten = 0, poisonApplesEaten = 0;
    int segmentsToGrow = 0, segmentsToShrink = 0;
    int lastLevelUpScore = 0;
    chrono::steady_clock::time_point startTime;
    vector<pair<Dir, Vec>> moveHistory; 
};

static atomic<Dir> inputDir;
static atomic<bool> inputQuit{false}, inputPause{false};
static KeyBindings currentKeys;

string gotorc(int r, int c) { return "\x1b[" + to_string(r) + ";" + to_string(c) + "H"; }
string clear() { return "\x1b[2J"; }
string hideCursor() { return "\x1b[?25l"; }
string showCursor() { return "\x1b[?25h"; }

#ifdef _WIN32
void enableVTAndUTF8() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
            SetConsoleMode(hOut, mode);
        }
    }
    SetConsoleOutputCP(65001);
}

pair<int,int> getTerminalSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    return {rows, cols};
}
#else
pair<int,int> getTerminalSize() { return {24, 80}; }
#endif

void calculateBoardSize(GameConfig& cfg) {
    auto [termRows, termCols] = getTerminalSize();

    int availableCols = (termCols - 35) / 2;
    int availableRows = termRows - 6;

    cfg.rows = max(10, min(availableRows, 35));
    cfg.cols = max(15, min(availableCols, 50));

    if (termCols < 100 || termRows < 30) {
        cfg.rows = max(10, min(cfg.rows, 16));
        cfg.cols = max(15, min(cfg.cols, 28));
    }

    if (cfg.rows % 2 == 0) cfg.rows--;
    if (cfg.cols % 2 == 0) cfg.cols--;
}

struct LayoutInfo {
    int originRow;
    int originCol;
    int panelCol;
    int termRows;
    int termCols;
    bool compactMode;
};

LayoutInfo calculateLayout(const GameConfig& cfg) {
    auto [termRows, termCols] = getTerminalSize();

    LayoutInfo layout;
    layout.termRows = termRows;
    layout.termCols = termCols;

    layout.compactMode = (termCols < 100 || termRows < 30);

    int boardWidth = cfg.cols * 2 + 4;
    int boardHeight = cfg.rows + 2;

    layout.originRow = max(2, (termRows - boardHeight) / 2);
    layout.originCol = max(2, (termCols - boardWidth - 30) / 2);
    layout.panelCol = layout.originCol + cfg.cols * 2 + 5;

    if (layout.panelCol + 25 > termCols) {
        layout.panelCol = termCols - 26;
    }

    return layout;
}

void sleepMs(int ms) { this_thread::sleep_for(chrono::milliseconds(ms)); }

bool isOpposite(Dir a, Dir b) {
    return (a == UP && b == DOWN) || (a == DOWN && b == UP) || (a == LEFT && b == RIGHT) || (a == RIGHT && b == LEFT);
}

Vec step(Vec v, Dir d) {
    if (d == UP) v.r -= 1;
    if (d == DOWN) v.r += 1;
    if (d == LEFT) v.c -= 1;
    if (d == RIGHT) v.c += 1;
    return v;
}

int urand(int lo, int hi) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dist(lo, hi);
    return dist(gen);
}

bool inBounds(const GameConfig& cfg, Vec v) {
    return v.r >= 0 && v.r < cfg.rows && v.c >= 0 && v.c < cfg.cols;
}

Vec wrapPosition(const GameConfig& cfg, Vec v) {
    if (v.r < 0) v.r = cfg.rows - 1;
    if (v.r >= cfg.rows) v.r = 0;
    if (v.c < 0) v.c = cfg.cols - 1;
    if (v.c >= cfg.cols) v.c = 0;
    return v;
}

FoodType determineFoodType(const GameConfig& cfg) {
    int chance = urand(1, 100);
    if (chance <= cfg.poisonAppleChance) return POISON;
    else if (chance <= cfg.poisonAppleChance + cfg.goldenAppleChance) return GOLDEN;
    return NORMAL;
}

void placeFood(const GameConfig& cfg, GameState& st) {
    vector<pair<int,int>> freeCells;
    vector<vector<bool>> occ(cfg.rows, vector<bool>(cfg.cols, false));
    for (auto &p : st.snake) if (inBounds(cfg, p)) occ[p.r][p.c] = true;
    for (auto &o : st.obstacles) occ[o.first][o.second] = true;
    for (auto &d : st.dynamicObstacles) occ[d.pos.r][d.pos.c] = true;
    for (int r=0; r<cfg.rows; r++) for (int c=0; c<cfg.cols; c++) if (!occ[r][c]) freeCells.push_back({r,c});
    if (freeCells.empty()) { st.gameOver = true; return; }

    auto [r,c] = freeCells[urand(0, (int)freeCells.size()-1)];
    st.food = {r,c};
    st.foodType = determineFoodType(cfg);
}

void spawnPowerUp(const GameConfig& cfg, GameState& st) {
    if (st.powerUps.size() >= 2 || urand(1, 100) > 15) return;

    vector<pair<int,int>> freeCells;
    vector<vector<bool>> occ(cfg.rows, vector<bool>(cfg.cols, false));
    for (auto &p : st.snake) if (inBounds(cfg, p)) occ[p.r][p.c] = true;
    for (auto &o : st.obstacles) occ[o.first][o.second] = true;
    for (int r=0; r<cfg.rows; r++) for (int c=0; c<cfg.cols; c++) if (!occ[r][c]) freeCells.push_back({r,c});
    if (freeCells.empty()) return;

    auto [r,c] = freeCells[urand(0, (int)freeCells.size()-1)];
    PowerUpType types[] = {SLOW_MOTION, INVINCIBILITY, MAGNET};
    PowerUp pu;
    pu.pos = {r, c};
    pu.type = types[urand(0, 2)];
    pu.duration = 300;
    pu.active = false;
    st.powerUps.push_back(pu);
}

void loadLevelObstacles(const GameConfig& cfg, GameState& st, int level) {
    st.obstacles.clear();
    int numGroups = 5 + level * 3;

    switch(level) {
        case 1:
            for (int i = 0; i < numGroups; ++i) {
                int row = urand(2, cfg.rows - 3);
                int col = urand(2, cfg.cols - 3);
                for (int j = 0; j < 3; ++j) {
                    int nr = row + urand(-1, 1);
                    int nc = col + urand(-1, 1);
                    if (nr >= 1 && nr < cfg.rows-1 && nc >= 1 && nc < cfg.cols-1)
                        st.obstacles.insert({nr, nc});
                }
            }
            break;
        case 2:
            for (int i = 0; i < 3; ++i) {
                int row = urand(cfg.rows/4, 3*cfg.rows/4);
                int startCol = urand(2, cfg.cols/3);
                for (int c = startCol; c < min(cfg.cols - 2, startCol + cfg.cols/2); c += 2)
                    st.obstacles.insert({row, c});
            }
            break;
        case 3:
            for (int i = 0; i < 3; ++i) {
                int col = urand(cfg.cols/4, 3*cfg.cols/4);
                int startRow = urand(2, cfg.rows/3);
                for (int r = startRow; r < min(cfg.rows - 2, startRow + cfg.rows/2); r += 2)
                    st.obstacles.insert({r, col});
            }
            break;
        case 4:
            {
                int midRow = cfg.rows / 2, midCol = cfg.cols / 2;
                for (int c = midCol - 8; c <= midCol + 8; c += 2)
                    if (c >= 1 && c < cfg.cols - 1) st.obstacles.insert({midRow, c});
                for (int r = midRow - 6; r <= midRow + 6; r += 2)
                    if (r >= 1 && r < cfg.rows - 1) st.obstacles.insert({r, midCol});
            }
            break;
        case 5:
            for (int i = 0; i < 6; ++i) {
                int row = urand(2, cfg.rows - 3);
                int startCol = urand(2, cfg.cols/2);
                for (int c = startCol; c < min(cfg.cols - 2, startCol + cfg.cols/3); c += 2)
                    st.obstacles.insert({row, c});
            }
            for (int i = 0; i < 4; ++i) {
                int col = urand(3, cfg.cols - 3);
                int startRow = urand(2, cfg.rows/2);
                for (int r = startRow; r < min(cfg.rows - 2, startRow + cfg.rows/3); r += 2)
                    st.obstacles.insert({r, col});
            }
            break;
    }

    int sr = cfg.rows / 2, sc = cfg.cols / 2;
    for (int r = sr - 3; r <= sr + 3; ++r)
        for (int c = sc - 5; c <= sc + 5; ++c)
            st.obstacles.erase({r, c});
}

void spawnDynamicObstacle(const GameConfig& cfg, GameState& st) {
    if (st.dynamicObstacles.size() >= 3 || urand(1, 100) > 20) return;

    DynamicObstacle dob;
    dob.pos = {urand(2, cfg.rows-3), urand(2, cfg.cols-3)};
    dob.dir = (Dir)urand(0, 3);
    dob.speed = 3 + cfg.currentLevel;
    st.dynamicObstacles.push_back(dob);
}

void updateDynamicObstacles(const GameConfig& cfg, GameState& st, int frame) {
    for (auto& dob : st.dynamicObstacles) {
        if (frame % dob.speed != 0) continue;

        Vec newPos = step(dob.pos, dob.dir);
        if (!inBounds(cfg, newPos) || st.obstacles.count({newPos.r, newPos.c})) {
            dob.dir = (Dir)urand(0, 3);
        } else {
            dob.pos = newPos;
        }
    }
}

void drawCell(const GameConfig& cfg, int originRow, int originCol, Vec v, const string& glyph) {
    cout << gotorc(originRow + v.r, originCol + v.c*2) << glyph;
}

void drawBorder(const GameConfig& cfg, int originRow, int originCol) {
    if (!cfg.bordersEnabled) return;
    for (int c = -1; c <= cfg.cols; ++c) {
        cout << gotorc(originRow - 1, originCol + c * 2) << WALL;
        cout << gotorc(originRow + cfg.rows, originCol + c * 2) << WALL;
    }
    for (int r = 0; r < cfg.rows; ++r) {
        cout << gotorc(originRow + r, originCol - 2) << WALL;
        cout << gotorc(originRow + r, originCol + cfg.cols * 2) << WALL;
    }
}


void drawEnhancedUI(const GameConfig& cfg, int originRow, int originCol, const GameState& st, const LayoutInfo& layout) {
    int panelCol = layout.panelCol;
    int startRow = layout.originRow;

    cout << gotorc(startRow-1, panelCol) << Color::bg(17) << Color::BRIGHT_YELLOW << Color::BOLD
         << "╔═══════════════════════╗" << Color::RESET;
    cout << gotorc(startRow, panelCol) << Color::bg(17) << Color::BRIGHT_YELLOW << Color::BOLD
         << "║  🐍 SNAKE MASTER 🐍  ║" << Color::RESET;
    cout << gotorc(startRow+1, panelCol) << Color::bg(17) << Color::BRIGHT_YELLOW << Color::BOLD
         << "╚═══════════════════════╝" << Color::RESET;

    cout << gotorc(startRow+3, panelCol) << Color::bg(22) << Color::BRIGHT_WHITE << " ═══ STATS ═══════════ " << Color::RESET;
    cout << gotorc(startRow+4, panelCol) << Color::BRIGHT_GREEN << "💰 Score: " << Color::BRIGHT_WHITE << Color::BOLD << setw(5) << st.score << Color::RESET;
    cout << gotorc(startRow+5, panelCol) << Color::BRIGHT_CYAN << "📏 Length: " << Color::BRIGHT_WHITE << setw(4) << st.snake.size() << Color::RESET;
    cout << gotorc(startRow+6, panelCol) << Color::BRIGHT_MAGENTA << "🎯 Level: " << Color::BRIGHT_WHITE << cfg.currentLevel << "/" << cfg.maxLevel << "   " << Color::RESET;
    cout << gotorc(startRow+7, panelCol) << Color::BRIGHT_BLUE << "⚡ Speed: " << Color::BRIGHT_WHITE << cfg.currentFps << " FPS  " << Color::RESET;

    auto elapsed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - st.startTime).count();
    cout << gotorc(startRow+8, panelCol) << Color::YELLOW << "⏱ Time: " << Color::BRIGHT_WHITE
         << elapsed/60 << ":" << setfill('0') << setw(2) << elapsed%60 << setfill(' ') << "   " << Color::RESET;

    cout << gotorc(startRow+10, panelCol) << Color::bg(53) << Color::BRIGHT_WHITE << " ═══ POWER-UP ════════ " << Color::RESET;
    if (st.activePowerUp != NONE && st.powerUpTimer > 0) {
        string powerName = (st.activePowerUp == SLOW_MOTION) ? "⏱ SLOW MOTION" :
                          (st.activePowerUp == INVINCIBILITY) ? "🛡 SHIELD" : "🧲 MAGNET";
        int bars = (st.powerUpTimer * 10) / 300;
        cout << gotorc(startRow+11, panelCol) << Color::BRIGHT_YELLOW << powerName << Color::RESET;
        cout << gotorc(startRow+12, panelCol) << Color::BRIGHT_GREEN << "[";
        for (int i = 0; i < 10; ++i) cout << (i < bars ? "█" : "░");
        cout << "]" << Color::RESET;
    } else {
        cout << gotorc(startRow+11, panelCol) << Color::DIM << "No active power-up    " << Color::RESET;
        cout << gotorc(startRow+12, panelCol) << "                       ";
    }

    cout << gotorc(startRow+14, panelCol) << Color::bg(52) << Color::BRIGHT_WHITE << " ═══ COLLECTED ════════ " << Color::RESET;
    cout << gotorc(startRow+15, panelCol) << Color::BRIGHT_YELLOW << "⭐ Golden: " << Color::BRIGHT_WHITE << st.goldenApplesEaten << "   " << Color::RESET;
    cout << gotorc(startRow+16, panelCol) << Color::MAGENTA << "☠ Poison: " << Color::BRIGHT_WHITE << st.poisonApplesEaten << "   " << Color::RESET;

    cout << gotorc(startRow+18, panelCol) << Color::bg(17) << Color::BRIGHT_WHITE << " ═══ CONTROLS ═════════ " << Color::RESET;
    cout << gotorc(startRow+19, panelCol) << Color::WHITE << "WASD/Arrows: Move     " << Color::RESET;
    cout << gotorc(startRow+20, panelCol) << Color::YELLOW << "P: Pause  Q: Quit     " << Color::RESET;

    cout << gotorc(startRow+22, panelCol) << Color::bg(235) << Color::BRIGHT_WHITE << " ═══ LEGEND ═══════════ " << Color::RESET;
    cout << gotorc(startRow+23, panelCol) << Color::BRIGHT_RED << "◆" << Color::WHITE << "+10  " << Color::BRIGHT_YELLOW << "★" << Color::WHITE << "+50  " << Color::MAGENTA << "☠" << Color::WHITE << "-30" << Color::RESET;
    cout << gotorc(startRow+24, panelCol) << Color::BRIGHT_BLUE << "⏱" << Color::WHITE << "Slow " << Color::BRIGHT_CYAN << "🛡" << Color::WHITE << "Shield " << Color::BRIGHT_MAGENTA << "🧲" << Color::WHITE << "Magnet" << Color::RESET;

    cout << gotorc(startRow+26, panelCol) << Color::CYAN << "Mode: "
         << (cfg.bordersEnabled ? Color::BRIGHT_GREEN + "Walls" : Color::BRIGHT_YELLOW + "Wrap")
         << "       " << Color::RESET;
}

void showPauseDialog(const GameConfig& cfg, int originRow, int originCol) {
    int msgRow = originRow + cfg.rows/2 - 2, msgCol = originCol + cfg.cols - 10;
    cout << gotorc(msgRow, msgCol) << Color::bg(21) << Color::BRIGHT_WHITE << Color::BOLD << "                    " << Color::RESET;
    cout << gotorc(msgRow+1, msgCol) << Color::bg(21) << Color::BRIGHT_YELLOW << Color::BOLD << "    ⏸ PAUSED ⏸     " << Color::RESET;
    cout << gotorc(msgRow+2, msgCol) << Color::bg(21) << Color::BRIGHT_WHITE << "  Press P to Resume " << Color::RESET;
    cout << gotorc(msgRow+3, msgCol) << Color::bg(21) << Color::BRIGHT_WHITE << Color::BOLD << "                    " << Color::RESET;
    cout.flush();
}

void showLevelUpNotification(const GameConfig& cfg, int originRow, int originCol, int newLevel) {
    int msgRow = originRow + cfg.rows/2 - 3, msgCol = originCol + cfg.cols - 12;
    cout << gotorc(msgRow, msgCol) << Color::bg(22) << Color::BRIGHT_WHITE << Color::BOLD << "                      " << Color::RESET;
    cout << gotorc(msgRow+1, msgCol) << Color::bg(22) << Color::BRIGHT_YELLOW << Color::BOLD << "   🎉 LEVEL UP! 🎉   " << Color::RESET;
    cout << gotorc(msgRow+2, msgCol) << Color::bg(22) << Color::BRIGHT_CYAN << "    Level " << newLevel << " / 5      " << Color::RESET;
    cout << gotorc(msgRow+3, msgCol) << Color::bg(22) << Color::BRIGHT_GREEN << "   Speed Increased!  " << Color::RESET;
    cout << gotorc(msgRow+4, msgCol) << Color::bg(22) << Color::BRIGHT_WHITE << Color::BOLD << "                      " << Color::RESET;
    cout.flush();
    sleepMs(1200);
    for (int i = 0; i < 5; ++i) cout << gotorc(msgRow + i, msgCol) << "                      ";
}

void replayGame(const GameConfig& cfg, const GameState& st, int originRow, int originCol, const LayoutInfo& layout) {
    cout << clear() << hideCursor();
    cout << gotorc(2, 30) << Color::BRIGHT_CYAN << Color::BOLD << "🎬 REPLAY MODE 🎬" << Color::RESET;
    cout << gotorc(3, 25) << Color::WHITE << "Press any key to skip replay..." << Color::RESET;

    sleepMs(1000);

    GameState replaySt;
    replaySt.snake = st.snake;
    replaySt.obstacles = st.obstacles;
    replaySt.dir = RIGHT;

    drawBorder(cfg, originRow, originCol);
    for (int r = 0; r < cfg.rows; ++r)
        for (int c = 0; c < cfg.cols; ++c)
            cout << gotorc(originRow + r, originCol + c * 2) << EMPTY;

    for (auto& o : st.obstacles)
        drawCell(cfg, originRow, originCol, {o.first, o.second}, OBSTACLE);

    for (size_t i = 1; i < replaySt.snake.size(); ++i)
        drawCell(cfg, originRow, originCol, replaySt.snake[i], SNAKE_BODY);
    drawCell(cfg, originRow, originCol, replaySt.snake.front(), SNAKE_HEAD);
    cout.flush();

    deque<Vec> snake = replaySt.snake;

    for (size_t i = 0; i < st.moveHistory.size(); ++i) {
        if (_kbhit()) { _getch(); break; }

        Vec newHead = st.moveHistory[i].second;
        snake.push_front(newHead);

        while (snake.size() > st.snake.size()) {
            Vec tail = snake.back();
            drawCell(cfg, originRow, originCol, tail, EMPTY);
            snake.pop_back();
        }

        if (snake.size() > 1)
            drawCell(cfg, originRow, originCol, snake[1], SNAKE_BODY);
        drawCell(cfg, originRow, originCol, snake.front(), SNAKE_HEAD);

        cout.flush();
        sleepMs(60);
    }

    sleepMs(400);
}

void showAnimatedIntro(const GameConfig& cfg) {
    cout << clear() << hideCursor();
    vector<string> frames = {"Loading", "Loading.", "Loading..", "Loading..."};
    auto [rows, cols] = getTerminalSize();
    int startRow = max(3, rows/4);
    int startCol = max(5, cols/2 - 10);
    cout << gotorc(startRow, startCol) << Color::BRIGHT_CYAN << Color::BOLD << "SNAKE MASTER" << Color::RESET;
    for (int i = 0; i < 10; ++i) {
        cout << gotorc(startRow + 2, startCol) << Color::BRIGHT_GREEN << frames[i % 4] << "   " << Color::RESET;
        cout.flush();
        sleepMs(120);
    }
    cout << gotorc(startRow + 2, startCol) << Color::BRIGHT_YELLOW << "Ready!      " << Color::RESET;
    cout.flush();
    sleepMs(350);
}

void showTitleScreen(const GameConfig& cfg) {
    cout << clear() << hideCursor();
    vector<string> title = {
        "     ███████╗███╗   ██╗ █████╗ ██╗  ██╗███████╗",
        "     ██╔════╝████╗  ██║██╔══██╗██║ ██╔╝██╔════╝",
        "     ███████╗██╔██╗ ██║███████║█████╔╝ █████╗  ",
        "     ╚════██║██║╚██╗██║██╔══██║██╔═██╗ ██╔══╝  ",
        "     ███████║██║ ╚████║██║  ██║██║  ██╗███████╗",
        "     ╚══════╝╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝"
    };
    auto [rows, cols] = getTerminalSize();
    int startRow = max(2, rows/6);
    int startCol = max(2, cols/2 - 30);
    vector<string> colors = {Color::BRIGHT_GREEN, Color::GREEN, Color::YELLOW, Color::BRIGHT_YELLOW, Color::BRIGHT_CYAN, Color::CYAN};
    for (size_t i=0; i<title.size(); ++i) {
        cout << gotorc(startRow + i, startCol) << colors[i] << Color::BOLD << title[i] << Color::RESET;
        cout.flush();
        sleepMs(60);
    }
    cout << gotorc(startRow + 8, startCol + 4) << Color::BRIGHT_MAGENTA << "🎮 Ultimate Edition - Power-ups & Levels 🎮" << Color::RESET;
    cout << gotorc(startRow + 10, startCol + 13) << Color::WHITE << "Press any key to continue..." << Color::RESET;
    cout.flush();
    while (!_kbhit()) sleepMs(50);
    _getch();
}

struct HighScore {
    string name;
    int score, length, level;
};

vector<HighScore> loadHighScores() {
    vector<HighScore> scores;
    ifstream file("snake_scores.txt");
    if (file.is_open()) {
        string name;
        int score, length, level;
        while (file >> name >> score >> length >> level)
            scores.push_back({name, score, length, level});
        file.close();
    }
    return scores;
}

void saveHighScores(const vector<HighScore>& scores) {
    ofstream file("snake_scores.txt");
    if (file.is_open()) {
        for (const auto& hs : scores)
            file << hs.name << " " << hs.score << " " << hs.length << " " << hs.level << "\n";
        file.close();
    }
}

void addHighScore(int score, int length, int level) {
    auto scores = loadHighScores();
    cout << clear() << hideCursor();
    auto [rows, cols] = getTerminalSize();
    cout << gotorc(max(6, rows/3), max(10, cols/3)) << Color::BRIGHT_YELLOW << "🏆 New High Score! 🏆" << Color::RESET;
    cout << gotorc(max(8, rows/3 + 2), max(10, cols/3)) << Color::WHITE << "Enter your name (max 10 chars): " << Color::RESET;
    cout << showCursor();
    cout.flush();
    string name;
    cin >> name;
    if (name.length() > 10) name = name.substr(0, 10);
    scores.push_back({name, score, length, level});
    sort(scores.begin(), scores.end(), [](const HighScore& a, const HighScore& b) { return a.score > b.score; });
    if (scores.size() > 10) scores.resize(10);
    saveHighScores(scores);
    cout << hideCursor();
}

void showHighScores(const GameConfig& cfg) {
    cout << clear() << hideCursor();
    auto scores = loadHighScores();
    auto [rows, cols] = getTerminalSize();
    int startCol = max(12, cols/2 - 24);
    cout << gotorc(2, startCol) << Color::BRIGHT_YELLOW << Color::BOLD << "╔════════════════════════════════╗" << Color::RESET;
    cout << gotorc(3, startCol) << Color::BRIGHT_YELLOW << Color::BOLD << "      HIGH SCORES 🏆 TOP 10      " << Color::RESET;
    cout << gotorc(4, startCol) << Color::BRIGHT_YELLOW << Color::BOLD << "╚════════════════════════════════╝" << Color::RESET;
    if (scores.empty()) {
        cout << gotorc(7, startCol+3) << Color::WHITE << "No high scores yet!" << Color::RESET;
    } else {
        cout << gotorc(6, startCol) << Color::DIM << Color::CYAN << "Rank  Name        Score    Length  Level" << Color::RESET;
        cout << gotorc(7, startCol) << Color::DIM << "─────────────────────────────────────────" << Color::RESET;
        for (size_t i = 0; i < scores.size(); ++i) {
            string rankColor = (i == 0) ? Color::BRIGHT_YELLOW : (i == 1) ? Color::WHITE : Color::CYAN;
            cout << gotorc(8 + i, startCol) << rankColor << setw(2) << (i+1) << "."
                 << Color::BRIGHT_WHITE << setw(12) << scores[i].name
                 << Color::BRIGHT_GREEN << setw(8) << scores[i].score
                 << Color::BRIGHT_CYAN << setw(8) << scores[i].length
                 << Color::BRIGHT_MAGENTA << setw(6) << scores[i].level << Color::RESET;
        }
    }
    cout << gotorc(min(rows-2, 22), startCol) << Color::DIM << "Press any key to return..." << Color::RESET;
    cout.flush();
    while (!_kbhit()) sleepMs(50);
    _getch();
}

void showSettings(GameConfig& cfg) {
    cout << clear() << hideCursor();
    auto [rows, cols] = getTerminalSize();
    int startRow = max(3, rows/4);
    int startCol = max(10, cols/2 - 20);
    cout << gotorc(startRow, startCol) << Color::BRIGHT_YELLOW << Color::BOLD << "╔═══════════════════════╗" << Color::RESET;
    cout << gotorc(startRow+1, startCol) << Color::BRIGHT_YELLOW << Color::BOLD << "    GAME SETTINGS ⚙️    " << Color::RESET;
    cout << gotorc(startRow+2, startCol) << Color::BRIGHT_YELLOW << Color::BOLD << "╚═══════════════════════╝" << Color::RESET;
    cout << gotorc(startRow+4, startCol) << Color::BRIGHT_CYAN << "1. Customize Controls" << Color::RESET;
    cout << gotorc(startRow+6, startCol) << Color::BRIGHT_CYAN << "2. Toggle Borders" << Color::RESET;
    cout << gotorc(startRow+8, startCol) << Color::WHITE << "Current: " << (cfg.bordersEnabled ? Color::BRIGHT_GREEN + "ON (Walls)" : Color::BRIGHT_YELLOW + "OFF (Wrap)") << Color::RESET;
    cout << gotorc(startRow+10, startCol) << Color::BRIGHT_CYAN << "3. Return to Menu" << Color::RESET;
    cout << gotorc(startRow+12, startCol) << Color::YELLOW << "Select option (1-3): " << Color::RESET;
    cout.flush();

    while (!_kbhit()) sleepMs(50);
    char choice = _getch();

    if (choice == '1') {
        cout << gotorc(startRow+14, startCol) << Color::BRIGHT_GREEN << "Enter Up key: " << Color::RESET; cout.flush(); cfg.keys.up = tolower(_getch());
        cout << gotorc(startRow+15, startCol) << Color::BRIGHT_GREEN << "Enter Down key: " << Color::RESET; cout.flush(); cfg.keys.down = tolower(_getch());
        cout << gotorc(startRow+16, startCol) << Color::BRIGHT_GREEN << "Enter Left key: " << Color::RESET; cout.flush(); cfg.keys.left = tolower(_getch());
        cout << gotorc(startRow+17, startCol) << Color::BRIGHT_GREEN << "Enter Right key: " << Color::RESET; cout.flush(); cfg.keys.right = tolower(_getch());
        cout << gotorc(startRow+19, startCol) << Color::BRIGHT_YELLOW << "✓ Controls updated!" << Color::RESET; cout.flush(); sleepMs(900);
    } else if (choice == '2') {
        cfg.bordersEnabled = !cfg.bordersEnabled;
        cout << gotorc(startRow+14, startCol) << Color::BRIGHT_YELLOW << "✓ Border mode changed!" << Color::RESET; cout.flush(); sleepMs(900);
    }
}

int showMenu(GameConfig& cfg, GameState& st, LayoutInfo& layout) {
    while (true) {
        calculateBoardSize(cfg);
        layout = calculateLayout(cfg);
        cout << clear() << hideCursor();
        int centerRow = layout.termRows / 2 - 8;
        int centerCol = max(2, layout.termCols / 2 - 20);
        cout << gotorc(centerRow, centerCol) << Color::BRIGHT_YELLOW << Color::BOLD << "╔═══════════════╗" << Color::RESET;
        cout << gotorc(centerRow+1, centerCol) << Color::BRIGHT_YELLOW << Color::BOLD << "    MAIN MENU    " << Color::RESET;
        cout << gotorc(centerRow+2, centerCol) << Color::BRIGHT_YELLOW << Color::BOLD << "╚═══════════════╝" << Color::RESET;
        cout << gotorc(centerRow+4, centerCol) << Color::BRIGHT_GREEN << "1. " << Color::WHITE << "🎮 Play Game" << Color::RESET;
        cout << gotorc(centerRow+6, centerCol) << Color::BRIGHT_CYAN << "2. " << Color::WHITE << "🏆 High Scores" << Color::RESET;
        // cout << gotorc(centerRow+8, centerCol+3) << Color::BRIGHT_MAGENTA << "3. " << Color::WHITE << "🎖️  Achievements (removed)" << Color::RESET;
        cout << gotorc(centerRow+8, centerCol) << Color::BRIGHT_BLUE << "3. " << Color::WHITE << "⚙️  Settings" << Color::RESET;
        cout << gotorc(centerRow+10, centerCol) << Color::BRIGHT_RED << "4. " << Color::WHITE << "🚪 Exit" << Color::RESET;
        cout << gotorc(centerRow+13, centerCol-3) << Color::DIM << "Select an option (1-4): " << Color::RESET;
        cout.flush();

        while (!_kbhit()) sleepMs(50);
        char choice = _getch();

        switch (choice) {
            case '1': return 1;
            case '2': showHighScores(cfg); break;
            case '3': showSettings(cfg); break;
            case '4': return 5;
        }
    }
}

void drawBoardInitial(const GameConfig& cfg, const GameState& st, int originRow, int originCol) {
    cout << clear() << hideCursor();
    drawBorder(cfg, originRow, originCol);
    for (int r=0; r<cfg.rows; ++r)
        for (int c=0; c<cfg.cols; ++c)
            cout << gotorc(originRow + r, originCol + c*2) << EMPTY;
    for (auto &o : st.obstacles)
        drawCell(cfg, originRow, originCol, {o.first, o.second}, OBSTACLE);
}

void inputThreadFunc() {
    KeyBindings keys = currentKeys;
    while (!inputQuit.load()) {
        if (_kbhit()) {
            int ch = _getch();

#ifdef _WIN32
            if (ch == 224) {
                int ar = _getch();
                if (ar == 72) inputDir = UP;
                else if (ar == 80) inputDir = DOWN;
                else if (ar == 75) inputDir = LEFT;
                else if (ar == 77) inputDir = RIGHT;
                continue;
            }
#else
            if (ch == 27) {
                usleep(1000);
                int next1 = _getch();
                usleep(1000);
                int next2 = _getch();
                if (next1 == '[') {
                    if (next2 == 'A') inputDir = UP;
                    else if (next2 == 'B') inputDir = DOWN;
                    else if (next2 == 'D') inputDir = LEFT;
                    else if (next2 == 'C') inputDir = RIGHT;
                    continue;
                }
            }
#endif
            ch = tolower(ch);
            if (ch == currentKeys.quit) { inputQuit = true; break; }
            if (ch == currentKeys.pause) { inputPause = !inputPause.load(); }

            if (ch == currentKeys.up) inputDir = UP;
            else if (ch == currentKeys.down) inputDir = DOWN;
            else if (ch == currentKeys.left) inputDir = LEFT;
            else if (ch == currentKeys.right) inputDir = RIGHT;
        }
    }
}


bool showGameOverScreen(const GameConfig& cfg, int originRow, int originCol, GameState& st, const LayoutInfo& layout) {
    int msgRow = layout.originRow + cfg.rows/2 - 5, msgCol = layout.originCol + cfg.cols - 14;
    cout << gotorc(msgRow, msgCol) << Color::bg(52) << Color::BRIGHT_WHITE << Color::BOLD << "                        " << Color::RESET;
    cout << gotorc(msgRow+1, msgCol) << Color::bg(52) << Color::BRIGHT_WHITE << Color::BOLD << "     GAME OVER! 💀      " << Color::RESET;
    cout << gotorc(msgRow+2, msgCol) << Color::bg(52) << Color::BRIGHT_YELLOW << "  Score: " << st.score << "             " << Color::RESET;
    cout << gotorc(msgRow+3, msgCol) << Color::bg(52) << Color::BRIGHT_CYAN << "  Length: " << st.snake.size() << "             " << Color::RESET;
    cout << gotorc(msgRow+4, msgCol) << Color::bg(52) << Color::BRIGHT_MAGENTA << "  Level: " << cfg.currentLevel << "              " << Color::RESET;
    cout << gotorc(msgRow+5, msgCol) << Color::bg(52) << Color::BRIGHT_WHITE << Color::BOLD << "                        " << Color::RESET;
    cout << gotorc(msgRow+7, msgCol) << Color::bg(22) << Color::BRIGHT_WHITE << " R: Replay  Enter: Play " << Color::RESET;
    cout << gotorc(msgRow+8, msgCol) << Color::bg(88) << Color::BRIGHT_WHITE << " Other Key: Menu        " << Color::RESET;
    cout.flush();

    auto scores = loadHighScores();
    bool isHighScore = scores.size() < 10 || st.score > scores.back().score;

    while (!_kbhit()) sleepMs(50);
    int ch = tolower(_getch());
    if (ch == 'r') {
        if (isHighScore) addHighScore(st.score, st.snake.size(), cfg.currentLevel);
        replayGame(cfg, st, originRow, originCol, layout);
        return false;
    }
    if (ch == 13 || ch == 'w' || ch == 'a' || ch == 's' || ch == 'd') {
        if (isHighScore) addHighScore(st.score, st.snake.size(), cfg.currentLevel);
        return true;
    }
    if (isHighScore) addHighScore(st.score, st.snake.size(), cfg.currentLevel);
    return false;
}

void initGameState(const GameConfig& cfg, GameState& st) {
    st.snake.clear();
    st.obstacles.clear();
    st.dynamicObstacles.clear();
    st.powerUps.clear();
    st.gameOver = false;
    st.quit = false;
    st.score = 0;
    st.moves = 0;
    st.goldenApplesEaten = 0;
    st.poisonApplesEaten = 0;
    st.segmentsToGrow = 0;
    st.segmentsToShrink = 0;
    st.dir = RIGHT;
    st.paused = false;
    st.lastLevelUpScore = 0;
    st.activePowerUp = NONE;
    st.powerUpTimer = 0;
    st.startTime = chrono::steady_clock::now();
    st.moveHistory.clear();

    int sr = cfg.rows/2, sc = cfg.cols/2 - 2;
    st.snake.push_back({sr, sc+2});
    st.snake.push_back({sr, sc+1});
    st.snake.push_back({sr, sc});

    inputDir = st.dir;
    inputQuit = false;
    inputPause = false;

    loadLevelObstacles(cfg, st, cfg.currentLevel);
    placeFood(cfg, st);
}

void updateGameSpeed(GameConfig& cfg, const GameState& st) {
    int speedBoost = st.activePowerUp == SLOW_MOTION ? -3 : 0;
    int speedLevel = st.score / 100;
    cfg.currentFps = cfg.baseFps + speedLevel * 2 + speedBoost;
    cfg.currentFps = max(5, min(cfg.currentFps, 20));
}

void checkLevelUp(GameConfig& cfg, GameState& st, int originRow, int originCol, const LayoutInfo& layout) {
    int scoreForNextLevel = cfg.currentLevel * 200;
    if (st.score >= scoreForNextLevel && st.score > st.lastLevelUpScore && cfg.currentLevel < cfg.maxLevel) {
        cfg.currentLevel++;
        st.lastLevelUpScore = st.score;
        showLevelUpNotification(cfg, originRow, originCol, cfg.currentLevel);
        loadLevelObstacles(cfg, st, cfg.currentLevel);
        drawBoardInitial(cfg, st, originRow, originCol);
        string foodGlyph = (st.foodType == GOLDEN) ? FOOD_GOLDEN : (st.foodType == POISON) ? FOOD_POISON : FOOD_NORMAL;
        drawCell(cfg, originRow, originCol, st.food, foodGlyph);
        for (size_t i=1; i<st.snake.size(); ++i)
            drawCell(cfg, originRow, originCol, st.snake[i], SNAKE_BODY);
        drawCell(cfg, originRow, originCol, st.snake.front(), SNAKE_HEAD);
    }
}

void runGame(GameConfig cfg, GameState& st, LayoutInfo& layout) {
    currentKeys = cfg.keys;

    int originRow = layout.originRow;
    int originCol = layout.originCol;

    drawBoardInitial(cfg, st, originRow, originCol);
    string foodGlyph = (st.foodType == GOLDEN) ? FOOD_GOLDEN : (st.foodType == POISON) ? FOOD_POISON : FOOD_NORMAL;
    drawCell(cfg, originRow, originCol, st.food, foodGlyph);
    for (size_t i=1; i<st.snake.size(); ++i)
        drawCell(cfg, originRow, originCol, st.snake[i], SNAKE_BODY);
    drawCell(cfg, originRow, originCol, st.snake.front(), SNAKE_HEAD);
    drawEnhancedUI(cfg, originRow, originCol, st, layout);
    cout.flush();

    thread inputThread(inputThreadFunc);
    auto last = chrono::steady_clock::now();
    int frameCount = 0;

    while (!st.gameOver && !st.quit) {
        bool wasPaused = st.paused;
        st.paused = inputPause.load();
        if (st.paused) {
            if (!wasPaused) showPauseDialog(cfg, originRow, originCol);
            sleepMs(50);
            continue;
        } else if (wasPaused && !st.paused) {
            drawBoardInitial(cfg, st, originRow, originCol);
            foodGlyph = (st.foodType == GOLDEN) ? FOOD_GOLDEN : (st.foodType == POISON) ? FOOD_POISON : FOOD_NORMAL;
            drawCell(cfg, originRow, originCol, st.food, foodGlyph);
            for (auto& pu : st.powerUps)
                drawCell(cfg, originRow, originCol, pu.pos, (pu.type == SLOW_MOTION) ? POWERUP_SLOW : (pu.type == INVINCIBILITY) ? POWERUP_SHIELD : POWERUP_MAGNET);
            for (auto& dob : st.dynamicObstacles)
                drawCell(cfg, originRow, originCol, dob.pos, DYNAMIC_OBSTACLE);
            for (size_t i=1; i<st.snake.size(); ++i)
                drawCell(cfg, originRow, originCol, st.snake[i], SNAKE_BODY);
            drawCell(cfg, originRow, originCol, st.snake.front(), st.activePowerUp == INVINCIBILITY ? SNAKE_INVINCIBLE : SNAKE_HEAD);
        }

        Dir nd = inputDir.load();
        if (!isOpposite(st.dir, nd)) st.dir = nd;
        if (inputQuit.load()) { st.quit = true; break; }

        updateGameSpeed(cfg, st);
        const int frame_ms = max(16, 1000 / cfg.currentFps);
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - last).count();
        if (elapsed < frame_ms) {
            sleepMs(frame_ms - elapsed);
            continue;
        }
        last = chrono::steady_clock::now();
        frameCount++;

        if (frameCount % 200 == 0 && st.score > 100) spawnDynamicObstacle(cfg, st);
        updateDynamicObstacles(cfg, st, frameCount);

        if (frameCount % 150 == 0) spawnPowerUp(cfg, st);

        Vec newHead = step(st.snake.front(), st.dir);
        if (cfg.bordersEnabled) {
            if (!inBounds(cfg, newHead)) st.gameOver = true;
        } else {
            newHead = wrapPosition(cfg, newHead);
        }

        if (!st.gameOver && st.activePowerUp != INVINCIBILITY) {
            if (st.obstacles.count({newHead.r, newHead.c})) st.gameOver = true;
            for (auto& dob : st.dynamicObstacles)
                if (dob.pos.r == newHead.r && dob.pos.c == newHead.c) st.gameOver = true;
            for (auto &seg : st.snake)
                if (seg.r == newHead.r && seg.c == newHead.c) { st.gameOver = true; break; }
        }
        if (st.gameOver) break;

        Vec tail = st.snake.back();
        st.snake.push_front(newHead);
        st.moves++;
        st.moveHistory.push_back({st.dir, newHead});

        bool ate = (newHead.r == st.food.r && newHead.c == st.food.c);
        if (ate) {
            if (st.foodType == GOLDEN) {
                st.score += cfg.goldenAppleScore;
                st.goldenApplesEaten++;
                st.segmentsToGrow += 2;
            } else if (st.foodType == POISON) {
                st.score += cfg.poisonApplePenalty;
                st.poisonApplesEaten++;
                st.segmentsToShrink += 2;
            } else {
                st.score += cfg.normalAppleScore;
                st.segmentsToGrow += 1;
            }
            placeFood(cfg, st);
            checkLevelUp(cfg, st, originRow, originCol, layout);
        }

        for (auto it = st.powerUps.begin(); it != st.powerUps.end();) {
            if (it->pos.r == newHead.r && it->pos.c == newHead.c) {
                st.activePowerUp = it->type;
                st.powerUpTimer = it->duration;
                it = st.powerUps.erase(it);
            } else {
                ++it;
            }
        }

        if (st.powerUpTimer > 0) {
            st.powerUpTimer--;
            if (st.powerUpTimer == 0) st.activePowerUp = NONE;
        }

        if (st.activePowerUp == MAGNET && frameCount % 10 == 0) {
            Vec foodDir = {0, 0};
            if (st.food.r < newHead.r) foodDir.r = 1;
            else if (st.food.r > newHead.r) foodDir.r = -1;
            if (st.food.c < newHead.c) foodDir.c = 1;
            else if (st.food.c > newHead.c) foodDir.c = -1;

            Vec newFood = {st.food.r + foodDir.r, st.food.c + foodDir.c};
            if (inBounds(cfg, newFood) && !st.obstacles.count({newFood.r, newFood.c})) {
                bool blocked = false;
                for (auto &seg : st.snake) if (seg.r == newFood.r && seg.c == newFood.c) { blocked = true; break; }
                if (!blocked) {
                    drawCell(cfg, originRow, originCol, st.food, EMPTY);
                    st.food = newFood;
                }
            }
        }

        if (st.segmentsToGrow > 0) {
            st.segmentsToGrow--;
        } else if (st.segmentsToShrink > 0 && st.snake.size() > 3) {
            st.segmentsToShrink--;
            st.snake.pop_back();
            if (!st.snake.empty()) {
                Vec extraTail = st.snake.back();
                drawCell(cfg, originRow, originCol, extraTail, EMPTY);
            }
            st.snake.pop_back();
            drawCell(cfg, originRow, originCol, tail, EMPTY);
            if (st.snake.size() <= 1) { st.gameOver = true; break; }
        } else {
            st.snake.pop_back();
            drawCell(cfg, originRow, originCol, tail, EMPTY);
        }

        for (auto& dob : st.dynamicObstacles)
            drawCell(cfg, originRow, originCol, dob.pos, DYNAMIC_OBSTACLE);
        for (auto& pu : st.powerUps) {
            string puGlyph = (pu.type == SLOW_MOTION) ? POWERUP_SLOW : (pu.type == INVINCIBILITY) ? POWERUP_SHIELD : POWERUP_MAGNET;
            drawCell(cfg, originRow, originCol, pu.pos, puGlyph);
        }
        if (st.snake.size() > 1)
            drawCell(cfg, originRow, originCol, st.snake[1], SNAKE_BODY);
        drawCell(cfg, originRow, originCol, st.snake.front(), st.activePowerUp == INVINCIBILITY ? SNAKE_INVINCIBLE : SNAKE_HEAD);
        if (ate && !st.gameOver) {
            foodGlyph = (st.foodType == GOLDEN) ? FOOD_GOLDEN : (st.foodType == POISON) ? FOOD_POISON : FOOD_NORMAL;
            drawCell(cfg, originRow, originCol, st.food, foodGlyph);
        }

        drawEnhancedUI(cfg, originRow, originCol, st, layout);
        cout.flush();
    }

    inputQuit = true;
    if (inputThread.joinable()) inputThread.join();
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    enableVTAndUTF8(); 

    GameConfig cfg;
    GameState st;
    LayoutInfo layout;

    calculateBoardSize(cfg);
    layout = calculateLayout(cfg);

    if (cfg.debugMode) {
        auto [r,c] = getTerminalSize();
        cout << clear() << hideCursor();
        cout << gotorc(2,2) << Color::BRIGHT_CYAN << "[DEBUG] Terminal: " << r << " rows x " << c << " cols" << Color::RESET;
        cout << gotorc(3,2) << Color::BRIGHT_GREEN << "[DEBUG] Calculated board: " << cfg.rows << " rows x " << cfg.cols << " cols" << Color::RESET;
        cout << gotorc(4,2) << Color::BRIGHT_YELLOW << "[DEBUG] Layout origin: " << layout.originRow << ", " << layout.originCol << Color::RESET;
        cout << gotorc(6,2) << Color::DIM << "Resizing will be applied when you (re)start a new game." << Color::RESET;
        cout.flush();
        sleepMs(1600);
    }

    showAnimatedIntro(cfg);
    showTitleScreen(cfg);

    while (true) {
        calculateBoardSize(cfg);
        layout = calculateLayout(cfg);
        int menuChoice = showMenu(cfg, st, layout);
        if (menuChoice == 5) break;
        else if (menuChoice == 1) {
            bool playAgain = true;
            cfg.currentLevel = 1;
            while (playAgain) {
                calculateBoardSize(cfg);
                layout = calculateLayout(cfg);
                initGameState(cfg, st);
                runGame(cfg, st, layout);
                if (!st.quit) {
                    playAgain = showGameOverScreen(cfg, layout.originRow, layout.originCol, st, layout);
                    if (playAgain) cfg.currentLevel = 1;
                } else {
                    playAgain = false;
                }
            }
        }
    }

    cout << showCursor() << clear() << gotorc(1, 1);
    cout << Color::BRIGHT_CYAN << "Thanks for playing Snake Master! 👋🐍" << Color::RESET << "\n\n";
    cout.flush();
    #ifndef _WIN32
        resetTerminal();
    #endif
    return 0;
}