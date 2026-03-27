#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>

const std::string HIGHSCORE_FILE = "/home/ntust/Ivan/minigame/highscores.txt";

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

enum Direction { UP, DOWN, LEFT, RIGHT };
enum Evolution { SNAKE, DRAGON, RAYQUAZA };

std::string RED = "\033[0;31m";
std::string GREEN = "\033[0;32m";
std::string BLUE = "\033[0;34m";
std::string YELLOW = "\033[1;33m";
std::string CYAN = "\033[0;36m";
std::string MAGENTA = "\033[0;35m";
std::string WHITE = "\033[1;37m";
std::string GRAY = "\033[0;90m";
std::string NC = "\033[0m";
std::string BOLD = "\033[1m";

int WIDTH = 60;
int HEIGHT = 25;

struct termios origTermios;
int stdinFd;

void setCursor(int x, int y) {
    std::cout << "\033[" << y << ";" << x << "H" << std::flush;
}

void clearScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
    std::cout << "\033[?25l" << std::flush;
}

void resetScreen() {
    std::cout << "\033[2J\033[H" << std::flush;
    std::cout << "\033[?25h" << std::flush;
    std::cout << "\033[0m" << std::flush;
    std::cout << "\n" << std::flush;
}

void disableRawMode() {
    tcsetattr(stdinFd, TCSAFLUSH, &origTermios);
}

void cleanup(int sig) {
    resetScreen();
    disableRawMode();
    exit(0);
}

void enableRawMode() {
    stdinFd = STDIN_FILENO;
    tcgetattr(stdinFd, &origTermios);
    struct termios raw = origTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(stdinFd, TCSAFLUSH, &raw);
    fcntl(stdinFd, F_SETFL, O_NONBLOCK);
}

int readKey() {
    char buf[10];
    int n = read(stdinFd, buf, sizeof(buf));
    if (n > 0) {
        if (buf[0] == 27 && n >= 3) {
            if (buf[1] == 91) {
                if (buf[2] == 'A') return 'U';
                if (buf[2] == 'B') return 'D';
                if (buf[2] == 'C') return 'R';
                if (buf[2] == 'D') return 'L';
            }
        }
        return buf[0];
    }
    return -1;
}

class SnakeGame {
private:
    std::vector<Point> snake;
    Direction direction;
    Direction nextDirection;
    Point food;
    std::vector<Point> obstacles;
    std::vector<Point> enemies;
    int score;
    Evolution evolution;
    int speed;
    bool gameOver;
    bool paused;
    bool invincible;
    int invincibleTime;
    bool dragonDanceReady;
    bool dragonDashReady;
    bool wallPassReady;
    bool bodyWarning;
    bool enemyPaused;
    time_t enemyPauseTime;
    time_t gameStartTime;
    time_t lastObstacleTime;
    time_t lastEnemyTime;
    int enemyCount;
    int obstacleCount;

public:
    SnakeGame() {
        srand(time(NULL));
    }

    void initGame() {
        snake.clear();
        snake.push_back({WIDTH/2, HEIGHT/2});
        snake.push_back({WIDTH/2-1, HEIGHT/2});
        snake.push_back({WIDTH/2-2, HEIGHT/2});

        direction = RIGHT;
        nextDirection = RIGHT;
        score = 0;
        evolution = SNAKE;
        speed = 20000;
        gameOver = false;
        paused = false;
        invincible = false;
        invincibleTime = 0;
        dragonDanceReady = true;
        dragonDashReady = true;
        wallPassReady = true;
        bodyWarning = false;
        enemyPaused = false;
        enemyPauseTime = 0;
        obstacles.clear();
        enemies.clear();
        enemyCount = 0;
        obstacleCount = 0;

        time(&gameStartTime);
        lastObstacleTime = gameStartTime;
        lastEnemyTime = gameStartTime;

        spawnFood();
    }

    void spawnFood() {
        while (true) {
            food.x = rand() % (WIDTH - 2) + 1;
            food.y = rand() % (HEIGHT - 2) + 1;

            bool collision = false;
            for (const auto& s : snake) {
                if (s.x == food.x && s.y == food.y) {
                    collision = true;
                    break;
                }
            }

            if (!collision) {
                for (const auto& obs : obstacles) {
                    if (obs.x == food.x && obs.y == food.y) {
                        collision = true;
                        break;
                    }
                }
            }

            if (!collision) break;
        }
    }

    void spawnObstacle() {
        while (true) {
            Point obs;
            obs.x = rand() % (WIDTH - 2) + 1;
            obs.y = rand() % (HEIGHT - 2) + 1;

            if (obs.x == WIDTH/2 && obs.y == HEIGHT/2) continue;

            bool inSafeZone = false;
            if (evolution == SNAKE) {
                int headX = snake[0].x;
                int headY = snake[0].y;
                if (abs(obs.x - headX) <= 4 && abs(obs.y - headY) <= 4) {
                    inSafeZone = true;
                }
            }
            if (inSafeZone) continue;

            bool collision = false;
            for (const auto& s : snake) {
                if (s.x == obs.x && s.y == obs.y) {
                    collision = true;
                    break;
                }
            }

            if (!collision) {
                obstacles.push_back(obs);
                obstacleCount++;
                break;
            }
        }
    }

    void spawnEnemy() {
        while (true) {
            Point enemy;
            enemy.x = rand() % (WIDTH - 2) + 1;
            enemy.y = rand() % (HEIGHT - 2) + 1;

            bool collision = false;
            for (const auto& s : snake) {
                if (s.x == enemy.x && s.y == enemy.y) {
                    collision = true;
                    break;
                }
            }

            if (!collision) {
                for (const auto& obs : obstacles) {
                    if (obs.x == enemy.x && obs.y == enemy.y) {
                        collision = true;
                        break;
                    }
                }
            }

            if (!collision) {
                enemies.push_back(enemy);
                enemyCount++;
                break;
            }
        }
    }

    void moveEnemies() {
        std::vector<Point> newEnemies;

        for (const auto& enemy : enemies) {
            Point e = enemy;
            int dx = 0, dy = 0;

            if (e.x < snake[0].x) dx = 1;
            else if (e.x > snake[0].x) dx = -1;

            if (e.y < snake[0].y) dy = 1;
            else if (e.y > snake[0].y) dy = -1;

            if (rand() % 2 == 0 && dx != 0) {
                dy = 0;
            } else if (dy != 0) {
                dx = 0;
            }

            if (evolution >= DRAGON) {
                e.x += dx;
                e.y += dy;
            } else {
                if (rand() % 3 != 0) {
                    e.x += dx;
                    e.y += dy;
                }
            }

            bool hitObstacle = false;
            if (e.x >= 0 && e.x < WIDTH && e.y >= 0 && e.y < HEIGHT) {
                for (const auto& obs : obstacles) {
                    if (obs.x == e.x && obs.y == e.y) {
                        hitObstacle = true;
                        break;
                    }
                }
                if (!hitObstacle) {
                    newEnemies.push_back(e);
                }
            }
        }

        enemies = newEnemies;
    }

    void checkCollisions() {
        Point head = snake[0];

        if (evolution < RAYQUAZA || !wallPassReady) {
            if (head.x < 0 || head.x > WIDTH-1 || head.y < 0 || head.y > HEIGHT-2) {
                if (evolution == RAYQUAZA && wallPassReady) {
                    if (head.x < 0) snake[0].x = WIDTH - 2;
                    else if (head.x > WIDTH-1) snake[0].x = 1;
                    else if (head.y < 0) snake[0].y = HEIGHT - 3;
                    else if (head.y > HEIGHT-2) snake[0].y = 1;
                    wallPassReady = false;
                } else {
                    gameOver = true;
                    return;
                }
            }
        } else {
            if (head.x < 0) snake[0].x = WIDTH - 2;
            else if (head.x > WIDTH-1) snake[0].x = 1;
            else if (head.y < 0) snake[0].y = HEIGHT - 3;
            else if (head.y > HEIGHT-2) snake[0].y = 1;
        }

        for (size_t i = 1; i < snake.size(); i++) {
            if (snake[i].x == snake[0].x && snake[i].y == snake[0].y) {
                gameOver = true;
                return;
            }
        }

        for (auto it = obstacles.begin(); it != obstacles.end(); ) {
            if (it->x == snake[0].x && it->y == snake[0].y) {
                if (evolution >= DRAGON) {
                    score += 2;
                    checkEvolution();
                    it = obstacles.erase(it);
                } else {
                    gameOver = true;
                    return;
                }
            } else {
                ++it;
            }
        }

        for (auto it = enemies.begin(); it != enemies.end(); ) {
            if (it->x == snake[0].x && it->y == snake[0].y) {
                if (invincible) {
                    ++it;
                } else if (evolution >= DRAGON) {
                    score += 3;
                    checkEvolution();
                    it = enemies.erase(it);
                } else {
                    gameOver = true;
                    return;
                }
            } else {
                ++it;
            }
        }
        
        bodyWarning = false;
        for (const auto& enemy : enemies) {
            for (size_t i = 1; i < snake.size(); i++) {
                if (enemy.x == snake[i].x && enemy.y == snake[i].y) {
                    bodyWarning = true;
                    break;
                }
            }
            if (bodyWarning) break;
        }

        if (snake[0].x == food.x && snake[0].y == food.y) {
            score++;
            checkEvolution();

            enemyPaused = true;
            time(&enemyPauseTime);
            
            std::vector<Point> remainingEnemies;
            for (const auto& enemy : enemies) {
                int dx = abs(enemy.x - food.x);
                int dy = abs(enemy.y - food.y);
                if (dx > 1 || dy > 1) {
                    remainingEnemies.push_back(enemy);
                } else {
                    score += 1;
                }
            }
            enemies = remainingEnemies;

            if (score % 3 == 0 && score > 0) {
                speed = std::max(5000, speed - 1000);
            }

            snake.push_back(snake.back());
            spawnFood();
        }
        
        if (enemyPaused) {
            time_t now;
            time(&now);
            if (now - enemyPauseTime >= 1) {
                enemyPaused = false;
            }
        }
    }

    void checkEvolution() {
        if (score == 10 && evolution == SNAKE) {
            evolution = DRAGON;
            speed = 130000;
            drawEvolutionAnimation("龍 (DRAGON)");
        } else if (score == 25 && evolution == DRAGON) {
            evolution = RAYQUAZA;
            speed = 100000;
            drawEvolutionAnimation("烈空座 (RAYQUAZA)");
        }
    }

    void drawEvolutionAnimation(const std::string& evoName) {
        for (int i = 0; i < 5; i++) {
            setCursor(1, 1);
            std::cout << YELLOW << "╔════════════════════════════════════════╗" << NC << "\n";
            std::cout << YELLOW << "║         進化中... " << evoName << "         ║" << NC << "\n";
            std::cout << YELLOW << "╚════════════════════════════════════════╝" << NC << std::flush;
            usleep(100000);

            setCursor(1, 1);
            std::cout << CYAN << "╔════════════════════════════════════════╗" << NC << "\n";
            std::cout << CYAN << "║         進化中... " << evoName << "         ║" << NC << "\n";
            std::cout << CYAN << "╚════════════════════════════════════════╝" << NC << std::flush;
            usleep(100000);
        }
    }

    void moveSnake() {
        direction = nextDirection;

        Point newHead = snake[0];
        switch (direction) {
            case UP:    newHead.y--; break;
            case DOWN:  newHead.y++; break;
            case LEFT:  newHead.x--; break;
            case RIGHT: newHead.x++; break;
        }

        snake.insert(snake.begin(), newHead);
        snake.pop_back();
    }

    void drawGame() {
        setCursor(1, 1);

        std::cout << GRAY << "┌";
        for (int x = 0; x < WIDTH-1; x++) std::cout << "-";
        std::cout << "┐" << NC << "\n";

        for (int y = 1; y < HEIGHT-2; y++) {
            std::cout << GRAY << "│" << NC;
            for (int x = 1; x < WIDTH-1; x++) {
                std::cout << " ";
            }
            std::cout << GRAY << "│" << NC << "\n";
        }

        std::cout << GRAY << "├";
        for (int x = 0; x < WIDTH-1; x++) std::cout << "-";
        std::cout << "┤" << NC << "\n";

        setCursor(food.x, food.y + 1);
        if (evolution == RAYQUAZA) {
            std::cout << YELLOW << "★" << NC;
        } else if (evolution == DRAGON) {
            std::cout << BLUE << "◆" << NC;
        } else {
            std::cout << GREEN << "●" << NC;
        }

        for (const auto& obs : obstacles) {
            setCursor(obs.x, obs.y + 1);
            std::cout << GRAY << "█" << NC;
        }

        for (const auto& enemy : enemies) {
            setCursor(enemy.x, enemy.y + 1);
            std::cout << RED << "✸" << NC;
        }

        for (size_t i = 0; i < snake.size(); i++) {
            setCursor(snake[i].x, snake[i].y + 1);
            if (i == 0) {
                if (evolution == RAYQUAZA) {
                    std::cout << CYAN << "⚡" << NC;
                } else if (evolution == DRAGON) {
                    std::cout << BLUE << "◎" << NC;
                } else {
                    std::cout << GREEN << "●" << NC;
                }
            } else {
                if (evolution == RAYQUAZA) {
                    std::cout << CYAN << "▼" << NC;
                } else if (evolution == DRAGON) {
                    std::cout << BLUE << "○" << NC;
                } else {
                    std::cout << GREEN << "○" << NC;
                }
            }
        }

        std::string stageName;
        std::string stageColor;
        if (evolution == SNAKE) {
            stageName = "綠毛蟲";
            stageColor = GREEN;
        } else if (evolution == DRAGON) {
            stageName = "逗逗龍";
            stageColor = BLUE;
        } else {
            stageName = "烈空座";
            stageColor = CYAN;
        }

        std::string hud = "分數: " + std::to_string(score) + " | 形態: " + stageName;
        
        if (bodyWarning) {
            hud += " | " + RED + "⚠ 敵人靠近!" + NC;
        }
        
        if (evolution == DRAGON) {
            hud += " | " + BLUE + "E-龍衝:" + std::string(dragonDashReady ? "就緒" : "冷卻") + NC;
        } else if (evolution == RAYQUAZA) {
            hud += " | " + MAGENTA + "SPACE-龍波:" + std::string(dragonDanceReady ? "就緒" : "冷卻") + NC;
            hud += " | " + RED + "Q-暴風:" + std::string(invincible ? "使用中" : "就緒") + NC;
            hud += " | " + CYAN + "穿牆:" + std::string(wallPassReady ? "就緒" : "冷卻") + NC;
        }
        
        setCursor(2, HEIGHT);
        std::cout << GRAY << "│" << NC << " " << hud;
        for (int x = hud.length() + 3; x < WIDTH-1; x++) std::cout << " ";
        std::cout << GRAY << "│" << NC << "\n";
        
        std::string info = "障礙:" + std::to_string(obstacles.size()) + " 敵人:" + std::to_string(enemies.size());
        if (enemyPaused) {
            info += " | " + YELLOW + "敵人暫停中!" + NC;
        }
        if (evolution == SNAKE) {
            info += " | 吃食清9宮";
        } else if (evolution == DRAGON) {
            info += " | 穿敵毀障";
        } else {
            info += " | 全技能";
        }
        std::cout << GRAY << "│" << NC << " " << info;
        for (int x = info.length() + 3; x < WIDTH-1; x++) std::cout << " ";
        std::cout << GRAY << "│" << NC << "\n";
        
        std::cout << GRAY << "└";
        for (int x = 0; x < WIDTH-1; x++) std::cout << "-";
        std::cout << "┘" << NC << "\n" << std::flush;
    }

    void handleInput() {
        int key = readKey();

        if (key == -1) return;

        switch (key) {
            case 'w': case 'W':
                if (direction != DOWN) nextDirection = UP;
                break;
            case 's': case 'S':
                if (direction != UP) nextDirection = DOWN;
                break;
            case 'a': case 'A':
                if (direction != RIGHT) nextDirection = LEFT;
                break;
            case 'd': case 'D':
                if (direction != LEFT) nextDirection = RIGHT;
                break;
            case 'p': case 'P':
                paused = !paused;
                break;
            case ' ':
                if (evolution == RAYQUAZA && dragonDanceReady) {
                    dragonDanceReady = false;
                    enemies.clear();
                    score += 5;
                    sleep(3);
                    dragonDanceReady = true;
                }
                break;
            case 'q': case 'Q':
                if (evolution == RAYQUAZA && !invincible) {
                    invincible = true;
                    invincibleTime = 3;
                    while (invincibleTime > 0) {
                        sleep(1);
                        invincibleTime--;
                    }
                    invincible = false;
                }
                break;
            case 'e': case 'E':
                if (evolution == DRAGON && dragonDashReady) {
                    dragonDashReady = false;
                    for (auto it = enemies.begin(); it != enemies.end(); ) {
                        int dist = abs(it->x - snake[0].x) + abs(it->y - snake[0].y);
                        if (dist <= 5) {
                            it = enemies.erase(it);
                            score += 2;
                        } else {
                            ++it;
                        }
                    }
                    sleep(3);
                    dragonDashReady = true;
                }
                break;
            default:
                break;
        }
    }

    void saveScore() {
        std::ifstream infile(HIGHSCORE_FILE);
        std::vector<std::pair<int, std::string>> scores;

        if (infile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                std::istringstream iss(line);
                int sc;
                std::string nm;
                if (iss >> sc >> nm) {
                    scores.push_back({sc, nm});
                }
            }
            infile.close();
        }

        if (scores.size() < 3) {
            while (scores.size() < 3) {
                scores.push_back({0, "匿名"});
            }
        }

        std::string name = getenv("USER") ? getenv("USER") : "玩家";

        bool inserted = false;
        std::vector<std::pair<int, std::string>> newScores;

        for (const auto& s : scores) {
            if (!inserted && score > s.first) {
                newScores.push_back({score, name});
                inserted = true;
            }
            if (newScores.size() < 5) {
                newScores.push_back(s);
            }
        }

        if (!inserted && newScores.size() < 5) {
            newScores.push_back({score, name});
        }

        std::ofstream outfile(HIGHSCORE_FILE);
        if (outfile.is_open()) {
            for (size_t i = 0; i < std::min(newScores.size(), (size_t)5); i++) {
                outfile << newScores[i].first << " " << newScores[i].second << "\n";
            }
            outfile.close();
        }
    }

    void showLeaderboard() {
        clearScreen();

        std::cout << CYAN << "\n";
        std::cout << "╔═══════════════════════════════╗\n";
        std::cout << "║        排行榜           ║\n";
        std::cout << "╚═══════════════════════════════╝\n";
        std::cout << NC << "\n";

        std::ifstream infile(HIGHSCORE_FILE);
        std::vector<std::pair<int, std::string>> scores;

        if (infile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                std::istringstream iss(line);
                int sc;
                std::string nm;
                if (iss >> sc >> nm) {
                    scores.push_back({sc, nm});
                }
            }
            infile.close();
        }

        int rank = 1;
        for (const auto& s : scores) {
            if (s.first > 0) {
                if (rank == 1) {
                    std::cout << YELLOW << "  1. " << s.second << ": " << s.first << " 分" << NC << "\n";
                } else if (rank == 2) {
                    std::cout << GRAY << "  2. " << s.second << ": " << s.first << " 分" << NC << "\n";
                } else if (rank == 3) {
                    std::cout << MAGENTA << "  3. " << s.second << ": " << s.first << " 分" << NC << "\n";
                } else {
                    std::cout << "  " << rank << ". " << s.second << ": " << s.first << " 分\n";
                }
                rank++;
            }
        }

        if (rank == 1) {
            std::cout << "  還沒有記錄!\n";
        }

        std::cout << "\n  按Enter返回...";
        std::cout.flush();

        disableRawMode();
        while (getchar() != '\n') {}
        enableRawMode();
    }

    void showGameOver() {
        resetScreen();

        if (gameOver) {
            std::cout << "\n" << RED;
            std::cout << "  遊戲結束!\n";
            std::cout << NC;
        }

        std::string stageName;
        if (evolution == SNAKE) stageName = "綠毛蟲";
        else if (evolution == DRAGON) stageName = "逗逗龍";
        else stageName = "烈空座";

        std::cout << "\n  " << BOLD << "最終分數: " << score << NC << "\n";
        std::cout << "  最終形態: " << stageName << "\n\n";

        saveScore();

        std::cout << CYAN << "  [1]" << NC << " 查看排行榜  ";
        std::cout << CYAN << "[2]" << NC << " 再來一局  ";
        std::cout << CYAN << "[3]" << NC << " 結束遊戲\n";
        std::cout << "\n  選擇: ";
        std::cout.flush();

        disableRawMode();
        int choice = getchar();
        while (choice != '\n' && choice != EOF) {
            if (choice >= '1' && choice <= '3') break;
            choice = getchar();
        }
        enableRawMode();

        switch (choice) {
            case '1':
                showLeaderboard();
                drawTitle();
                gameLoop();
                break;
            case '2':
                gameLoop();
                break;
            default:
                break;
        }
    }

    void drawTitle() {
        clearScreen();

        std::cout << CYAN;
        std::cout << "    ╔═══════════════════════════════════════════════╗\n";
        std::cout << "    ║                                               ║\n";
        std::cout << "    ║     🐉 貪吃蛇：烈空座進化 🐉                 ║\n";
        std::cout << "    ║     SNAKE EVOLUTION: RAYQUAZA                 ║\n";
        std::cout << "    ║                                               ║\n";
        std::cout << "    ╚═══════════════════════════════════════════════╝\n";
        std::cout << NC << "\n";

        std::cout << YELLOW << "\n┌─ 控制說明 ─────────────────────────────────┐" << NC << "\n";
        std::cout << "│ " << GREEN << "W" << NC << " A " << GREEN << "S" << NC << " D " << GREEN << "      " << NC << "移動方向                       │\n";
        std::cout << "│ " << BLUE << "E" << NC << "                    龍衝(消除5格內敵人)   │\n";
        std::cout << "│ " << MAGENTA << "空白鍵" << NC << "             龍波動(清除所有敵人)   │\n";
        std::cout << "│ " << RED << "Q" << NC << "                    暴風驅散(無敵3秒)       │\n";
        std::cout << "│ " << GRAY << "P" << NC << "                    暫停遊戲                │\n";
        std::cout << YELLOW << "└────────────────────────────────────────────┘" << NC << "\n";

        std::cout << YELLOW << "\n┌─ 進化系統 ─────────────────────────────────┐" << NC << "\n";
        std::cout << "│ " << GREEN << "【綠毛蟲】" << NC << " 0-9 分                    │\n";
        std::cout << "│   能力：吃食物時，敵人暫停1秒 + 清除9宮格 │\n";
        std::cout << "│                                                │\n";
        std::cout << "│ " << BLUE << "【逗逗龍】" << NC << " 10-24 分                  │\n";
        std::cout << "│   能力：可穿透敵人、可摧毀障礙物(+2分)     │\n";
        std::cout << "│   技能：按E鍵龍衝，消除周圍5格內敵人       │\n";
        std::cout << "│                                                │\n";
        std::cout << "│ " << CYAN << "【烈空座】" << NC << " 25+ 分                   │\n";
        std::cout << "│   能力：可穿牆(冷卻5秒)                    │\n";
        std::cout << "│   技能：空白鍵龍波動、Q鍵暴風驅散         │\n";
        std::cout << YELLOW << "└────────────────────────────────────────────┘" << NC << "\n";

        std::cout << YELLOW << "\n┌─ 困難設計 ─────────────────────────────────┐" << NC << "\n";
        std::cout << "│ 每3秒生成障礙物(█)                         │\n";
        std::cout << "│ 每5秒生成敵人(✸)，敵人撞障礙物會消失      │\n";
        std::cout << "│ 每吃3個食物速度加快                        │\n";
        std::cout << YELLOW << "└────────────────────────────────────────────┘" << NC << "\n";

        std::cout << "\n  按Enter開始遊戲...";
        std::cout.flush();

        disableRawMode();
        while (getchar() != '\n') {}
        enableRawMode();
    }

    void gameLoop() {
        initGame();

        while (!gameOver) {
            if (paused) {
                setCursor(WIDTH/2 - 5, HEIGHT/2);
                std::cout << YELLOW << "暫停中..." << NC << std::flush;
                sleep(1);
                continue;
            }

            handleInput();

            if (gameOver) break;

            moveSnake();
            checkCollisions();

            if (gameOver) break;

            if (enemyCount > 0 && !enemyPaused) {
                moveEnemies();
                checkCollisions();
            }

            if (gameOver) break;

            time_t currentTime;
            time(&currentTime);

            if (currentTime - lastObstacleTime >= 3) {
                spawnObstacle();
                lastObstacleTime = currentTime;
            }

            if (currentTime - lastEnemyTime >= 5) {
                spawnEnemy();
                lastEnemyTime = currentTime;
            }

            if (!wallPassReady && evolution == RAYQUAZA) {
                sleep(3);
                wallPassReady = true;
            }

            drawGame();
            usleep(speed);
        }

        showGameOver();
    }

    void start() {
        enableRawMode();
        drawTitle();
        gameLoop();
        disableRawMode();
        resetScreen();
    }
};

int main() {
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    SnakeGame game;
    game.start();
    return 0;
}
