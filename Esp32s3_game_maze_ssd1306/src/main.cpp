#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Піни I2C для ESP32-S3
#define SDA_PIN 8
#define SCL_PIN 9

// Піни кнопок керування
#define BTN_UP 4
#define BTN_DOWN 5
#define BTN_LEFT 6
#define BTN_RIGHT 7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Розміри лабіринту (16x8 клітинок по 8x8 пікселів = 128x64)
const int COLS = 16;
const int ROWS = 8;
const int TILE_SIZE = 8;

// 1 - стіна, 0 - прохід з точкою, 2 - порожній прохід
uint8_t baseMap[ROWS][COLS] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1 },
    { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
    { 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

uint8_t currentMap[ROWS][COLS];

struct Entity {
    int x;
    int y;
    int dirX;
    int dirY;
};

Entity player;
Entity monsters[3];
int dotsRemaining = 0;
bool gameOver = false;
bool gameWon = false;

unsigned long lastMonsterMove = 0;
const int monsterSpeed = 400; // Затримка руху монстрів у мс
unsigned long lastInputTime = 0;

void initGame()
{
    dotsRemaining = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            currentMap[r][c] = baseMap[r][c];
            if (currentMap[r][c] == 0) {
                dotsRemaining++;
            }
        }
    }

    // Початкова позиція гравця
    player = { 1, 1, 0, 0 };
    currentMap[player.y][player.x] = 2; // Збираємо точку під гравцем
    dotsRemaining--;

    // Початкові позиції 3-х монстрів
    monsters[0] = { 14, 1, -1, 0 };
    monsters[1] = { 14, 6, 0, -1 };
    monsters[2] = { 7, 3, 1, 0 };

    gameOver = false;
    gameWon = false;
}

void setup()
{
    Wire.begin(SDA_PIN, SCL_PIN);

    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        for (;;)
            ; // Зупинка, якщо дисплей не знайдено
    }

    randomSeed(analogRead(0));
    initGame();
}

void handleInput()
{
    if (millis() - lastInputTime < 150)
        return; // Антибрязкіт та обмеження швидкості

    int nextX = player.x;
    int nextY = player.y;
    bool moved = false;

    if (digitalRead(BTN_UP) == LOW) {
        nextY--;
        moved = true;
    } else if (digitalRead(BTN_DOWN) == LOW) {
        nextY++;
        moved = true;
    } else if (digitalRead(BTN_LEFT) == LOW) {
        nextX--;
        moved = true;
    } else if (digitalRead(BTN_RIGHT) == LOW) {
        nextX++;
        moved = true;
    }

    if (moved) {
        lastInputTime = millis();
        if (gameOver || gameWon) {
            initGame(); // Перезапуск гри при натисканні будь-якої кнопки
            return;
        }

        // Перевірка колізії зі стіною
        if (currentMap[nextY][nextX] != 1) {
            player.x = nextX;
            player.y = nextY;

            // Збір точки
            if (currentMap[player.y][player.x] == 0) {
                currentMap[player.y][player.x] = 2;
                dotsRemaining--;
                if (dotsRemaining <= 0) {
                    gameWon = true;
                }
            }
        }
    }
}

void moveMonsters()
{
    if (millis() - lastMonsterMove < monsterSpeed)
        return;
    lastMonsterMove = millis();

    int dirs[4][2] = { { 0, -1 }, { 0, 1 }, { -1, 0 }, { 1, 0 } };

    for (int i = 0; i < 3; i++) {
        int nextX = monsters[i].x + monsters[i].dirX;
        int nextY = monsters[i].y + monsters[i].dirY;

        // Якщо попереду стіна або випадково на перехресті змінюємо напрямок
        if (currentMap[nextY][nextX] == 1 || random(0, 10) < 3) {
            int validDirs[4];
            int count = 0;

            for (int d = 0; d < 4; d++) {
                int tx = monsters[i].x + dirs[d][0];
                int ty = monsters[i].y + dirs[d][1];
                // Уникаємо стін і намагаємось не розвертатись назад без потреби
                if (currentMap[ty][tx] != 1) {
                    validDirs[count++] = d;
                }
            }

            if (count > 0) {
                int chosen = validDirs[random(0, count)];
                monsters[i].dirX = dirs[chosen][0];
                monsters[i].dirY = dirs[chosen][1];
            }
        }

        // Рух монстра
        if (currentMap[monsters[i].y + monsters[i].dirY][monsters[i].x + monsters[i].dirX] != 1) {
            monsters[i].x += monsters[i].dirX;
            monsters[i].y += monsters[i].dirY;
        }
    }
}

void checkCollisions()
{
    for (int i = 0; i < 3; i++) {
        if (player.x == monsters[i].x && player.y == monsters[i].y) {
            gameOver = true;
        }
    }
}

void drawGame()
{
    display.clearDisplay();

    if (gameOver || gameWon) {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(35, 20);
        display.println(gameWon ? "YOU WIN!" : "GAME OVER");
        display.setCursor(15, 38);
        display.println("Press any button");
        display.display();
        return;
    }

    // Малюємо лабіринт та точки
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            int px = c * TILE_SIZE;
            int py = r * TILE_SIZE;

            if (currentMap[r][c] == 1) {
                display.fillRect(px, py, TILE_SIZE, TILE_SIZE, SSD1306_WHITE);
            } else if (currentMap[r][c] == 0) {
                display.fillCircle(px + 3, py + 3, 1, SSD1306_WHITE);
            }
        }
    }

    // Малюємо гравця (коло)
    display.fillCircle(player.x * TILE_SIZE + 3, player.y * TILE_SIZE + 3, 3, SSD1306_WHITE);

    // Малюємо 3-х монстрів (трикутники або квадрати з вирізом)
    for (int i = 0; i < 3; i++) {
        int mx = monsters[i].x * TILE_SIZE;
        int my = monsters[i].y * TILE_SIZE;
        display.drawRect(mx + 1, my + 1, 6, 6, SSD1306_WHITE);
        display.drawPixel(mx + 2, my + 3, SSD1306_WHITE); // Очі монстра
        display.drawPixel(mx + 5, my + 3, SSD1306_WHITE);
    }

    display.display();
}

void loop()
{
    handleInput();

    if (!gameOver && !gameWon) {
        moveMonsters();
        checkCollisions();
    }

    drawGame();
}
