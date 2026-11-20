#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Піни для ESP32-S3
#define I2C_SDA 8
#define I2C_SCL 9
#define BTN_UP 4
#define BTN_DOWN 5
#define BTN_FIRE 6

// Налаштування гри
#define MAX_BULLETS 5
#define MAX_ENEMIES 6

// Структури об'єктів
struct Player {
    float y = SCREEN_HEIGHT / 2;
    int width = 6;
    int height = 10;
    int hp = 3;
};

struct Bullet {
    float x, y;
    int width = 4;
    int height = 2;
    bool active = false;
};

struct Enemy {
    float x, y;
    int width = 8;
    int height = 8;
    float speed;
    bool active = false;
};

Player player;
Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];

int score = 0;
bool gameOver = false;
unsigned long lastFireTime = 0;
unsigned long lastSpawnTime = 0;

// Функція перевірки колізій (перетин двох прямокутників - AABB)
bool checkCollision(float x1, float y1, int w1, int h1, float x2, float y2, int w2, int h2)
{
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

void resetGame()
{
    player.y = SCREEN_HEIGHT / 2;
    player.hp = 3;
    score = 0;
    gameOver = false;
    for (int i = 0; i < MAX_BULLETS; i++)
        bullets[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++)
        enemies[i].active = false;
}

void setup()
{
    Wire.begin(I2C_SDA, I2C_SCL);

    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_FIRE, INPUT_PULLUP);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        for (;;)
            ; // Зупинка, якщо дисплей не знайдено
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    randomSeed(analogRead(0));
}

void spawnEnemy()
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].active = true;
            enemies[i].x = SCREEN_WIDTH;
            // Спавн нижче верхньої панелі (від 12 пікселя)
            enemies[i].y = random(12, SCREEN_HEIGHT - enemies[i].height);
            enemies[i].speed = random(10, 25) / 10.0; // Швидкість від 1.0 до 2.5
            break;
        }
    }
}

void fireBullet()
{
    if (millis() - lastFireTime < 250)
        return; // Затримка між пострілами (250 мс)

    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = true;
            bullets[i].x = player.width + 2;
            bullets[i].y = player.y + (player.height / 2) - 1;
            lastFireTime = millis();
            break;
        }
    }
}

void updateGame()
{
    // 1. Керування гравцем
    if (digitalRead(BTN_UP) == LOW && player.y > 12) {
        player.y -= 2.5;
    }
    if (digitalRead(BTN_DOWN) == LOW && player.y < SCREEN_HEIGHT - player.height) {
        player.y += 2.5;
    }
    if (digitalRead(BTN_FIRE) == LOW) {
        fireBullet();
    }

    // 2. Спавн ворогів кожні 1.2 секунди
    if (millis() - lastSpawnTime > 1200) {
        spawnEnemy();
        lastSpawnTime = millis();
    }

    // 3. Рух снарядів
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += 4.0;
            if (bullets[i].x > SCREEN_WIDTH) {
                bullets[i].active = false;
            }
        }
    }

    // 4. Рух ворогів та перевірка колізій
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active)
            continue;

        enemies[i].x -= enemies[i].speed;

        // Колізія: Ворог дійшов до бази (лівий край) або врізався в гравця
        bool hitPlayer = checkCollision(2, player.y, player.width, player.height,
            enemies[i].x, enemies[i].y, enemies[i].width, enemies[i].height);

        if (enemies[i].x <= 0 || hitPlayer) {
            enemies[i].active = false;
            player.hp--;
            if (player.hp <= 0) {
                gameOver = true;
            }
            continue;
        }

        // Колізія: Снаряд влучив у ворога
        for (int j = 0; j < MAX_BULLETS; j++) {
            if (bullets[j].active) {
                if (checkCollision(bullets[j].x, bullets[j].y, bullets[j].width, bullets[j].height,
                        enemies[i].x, enemies[i].y, enemies[i].width, enemies[i].height)) {
                    bullets[j].active = false;
                    enemies[i].active = false;
                    score += 10;
                    break; // Переходимо до наступного ворога
                }
            }
        }
    }
}

void drawGame()
{
    display.clearDisplay();

    // Верхній HUD (рахунок та життя)
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("HP:");
    display.print(player.hp);
    display.setCursor(65, 0);
    display.print("SCR:");
    display.print(score);
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

    if (gameOver) {
        display.setTextSize(2);
        display.setCursor(10, 22);
        display.print("GAME OVER");
        display.setTextSize(1);
        display.setCursor(18, 45);
        display.print("Press FIRE to reset");
        display.display();
        return;
    }

    // Малюємо гравця (трикутник-корабель)
    display.fillTriangle(2, player.y,
        2, player.y + player.height,
        2 + player.width, player.y + (player.height / 2),
        SSD1306_WHITE);

    // Малюємо снаряди
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            display.fillRect(bullets[i].x, bullets[i].y, bullets[i].width, bullets[i].height, SSD1306_WHITE);
        }
    }

    // Малюємо ворогів
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active) {
            display.drawRect(enemies[i].x, enemies[i].y, enemies[i].width, enemies[i].height, SSD1306_WHITE);
            display.fillRect(enemies[i].x + 2, enemies[i].y + 2, enemies[i].width - 4, enemies[i].height - 4, SSD1306_WHITE);
        }
    }

    display.display();
}

void loop()
{
    if (gameOver) {
        drawGame();
        if (digitalRead(BTN_FIRE) == LOW) {
            delay(300); // Захист від брязкоту контактів
            resetGame();
        }
        return;
    }

    updateGame();
    drawGame();
    delay(20); // ~50 FPS
}
