#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <math.h>

// Налаштування дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Піни I2C для ESP32-S3
#define I2C_SDA 8
#define I2C_SCL 9

// Піни кнопок та звуку
#define BTN_UP 4
#define BTN_LEFT 5
#define BTN_RIGHT 6
#define BTN_FIRE 7
#define BUZZER_PIN 10

// Карта лабіринту (12x12)
#define MAP_W 12
#define MAP_H 12
const uint8_t worldMap[MAP_H][MAP_W] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
    { 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1 },
    { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
    { 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1 },
    { 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1 },
    { 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

// Параметри гравця
float posX = 1.5, posY = 1.5;
float dirX = 1.0, dirY = 0.0;
float planeX = 0.0, planeY = 0.66;
int playerHP = 100;
int score = 0;

// Структура ворога
struct Enemy {
    float x, y;
    bool alive;
    unsigned long lastAttack;
};

#define NUM_ENEMIES 3
Enemy enemies[NUM_ENEMIES] = {
    { 3.5, 3.5, true, 0 },
    { 8.5, 2.5, true, 0 },
    { 7.5, 9.5, true, 0 }
};

// Буфер глибини (Z-buffer) для правильного перекриття ворогів стінами
float zBuffer[SCREEN_WIDTH];

// Змінні для анімації пострілу та звуку
int shootAnim = 0;
unsigned long soundEndTime = 0;

void playSound(int freq, int duration)
{
    tone(BUZZER_PIN, freq);
    soundEndTime = millis() + duration;
}

void updateSound()
{
    if (soundEndTime > 0 && millis() > soundEndTime) {
        noTone(BUZZER_PIN);
        soundEndTime = 0;
    }
}

void setup()
{
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_FIRE, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        for (;;)
            ; // Зупинка, якщо дисплей не знайдено
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(20, 25);
    display.println("ESP32-S3 DOOM");
    display.display();

    playSound(440, 150);
    delay(200);
    playSound(880, 300);
    delay(1000);
}

void loop()
{
    updateSound();

    // Перевірка кінця гри
    if (playerHP <= 0) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(10, 20);
        display.println("GAME OVER");
        display.setTextSize(1);
        display.setCursor(35, 45);
        display.print("Score: ");
        display.println(score);
        display.display();
        return;
    }

    // --- 1. ОБРОБКА КЕРУВАННЯ ---
    float moveSpeed = 0.08;
    float rotSpeed = 0.07;

    if (digitalRead(BTN_UP) == LOW) {
        if (worldMap[int(posY)][int(posX + dirX * moveSpeed * 2)] == 0)
            posX += dirX * moveSpeed;
        if (worldMap[int(posY + dirY * moveSpeed * 2)][int(posX)] == 0)
            posY += dirY * moveSpeed;
    }
    if (digitalRead(BTN_LEFT) == LOW) {
        float oldDirX = dirX;
        dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
        dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
        float oldPlaneX = planeX;
        planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
        planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
    }
    if (digitalRead(BTN_RIGHT) == LOW) {
        float oldDirX = dirX;
        dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
        dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
        float oldPlaneX = planeX;
        planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
        planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
    }
    if (digitalRead(BTN_FIRE) == LOW && shootAnim == 0) {
        shootAnim = 5;
        playSound(150, 80); // Звук пострілу

        // Перевірка влучання у ворогів по центру екрана
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive)
                continue;
            float dx = enemies[i].x - posX;
            float dy = enemies[i].y - posY;
            float dist = sqrt(dx * dx + dy * dy);
            // Скалярний добуток для перевірки кута прицілювання
            float dot = (dx * dirX + dy * dirY) / dist;
            if (dot > 0.96 && dist < 6.0) { // Ворог у прицілі
                enemies[i].alive = false;
                score += 100;
                playSound(600, 150); // Звук знищення ворога
            }
        }
    }

    // --- 2. ЛОГІКА ВОРОГІВ ---
    for (int i = 0; i < NUM_ENEMIES; i++) {
        if (!enemies[i].alive)
            continue;
        float dx = posX - enemies[i].x;
        float dy = posY - enemies[i].y;
        float dist = sqrt(dx * dx + dy * dy);

        // Ворог рухається до гравця, якщо він в межах видимості
        if (dist < 5.0 && dist > 0.8) {
            float stepX = (dx / dist) * 0.025;
            float stepY = (dy / dist) * 0.025;
            if (worldMap[int(enemies[i].y)][int(enemies[i].x + stepX)] == 0)
                enemies[i].x += stepX;
            if (worldMap[int(enemies[i].y + stepY)][int(enemies[i].x)] == 0)
                enemies[i].y += stepY;
        }
        // Атака впритул
        else if (dist <= 0.8 && millis() - enemies[i].lastAttack > 1000) {
            playerHP -= 15;
            enemies[i].lastAttack = millis();
            playSound(100, 200); // Звук отримання шкоди
        }
    }

    // --- 3. РЕНДЕРИНГ СТІН (RAYCASTING) ---
    display.clearDisplay();

    // Малюємо лінію горизонту (підлогу)
    display.drawFastHLine(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SSD1306_WHITE);

    for (int x = 0; x < SCREEN_WIDTH; x++) {
        float cameraX = 2 * x / float(SCREEN_WIDTH) - 1;
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        int mapX = int(posX);
        int mapY = int(posY);

        float sideDistX, sideDistY;
        float deltaDistX = (rayDirX == 0) ? 1e30 : abs(1 / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30 : abs(1 / rayDirY);
        float perpWallDist;

        int stepX, stepY;
        int hit = 0;
        int side;

        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        // Алгоритм DDA
        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            if (worldMap[mapY][mapX] > 0)
                hit = 1;
        }

        if (side == 0)
            perpWallDist = (sideDistX - deltaDistX);
        else
            perpWallDist = (sideDistY - deltaDistY);

        zBuffer[x] = perpWallDist; // Зберігаємо відстань до стіни

        int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0)
            drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd >= SCREEN_HEIGHT)
            drawEnd = SCREEN_HEIGHT - 1;

        // Імітація тіней на монохромному екрані:
        // Стіни по осі X малюємо суцільними лініями, по осі Y — пунктиром
        if (side == 0) {
            display.drawFastVLine(x, drawStart, drawEnd - drawStart, SSD1306_WHITE);
        } else {
            for (int y = drawStart; y < drawEnd; y += 2) {
                display.drawPixel(x, y, SSD1306_WHITE);
            }
        }
    }

    // --- 4. РЕНДЕРИНГ ВОРОГІВ (СПРАЙТИ) ---
    for (int i = 0; i < NUM_ENEMIES; i++) {
        if (!enemies[i].alive)
            continue;

        float spriteX = enemies[i].x - posX;
        float spriteY = enemies[i].y - posY;

        float invDet = 1.0 / (planeX * dirY - dirX * planeY);
        float transformX = invDet * (dirY * spriteX - dirX * spriteY);
        float transformY = invDet * (-planeY * spriteX + planeX * spriteY);

        if (transformY > 0.1) { // Ворог перед камерою
            int spriteScreenX = int((SCREEN_WIDTH / 2) * (1 + transformX / transformY));
            int spriteHeight = abs(int(SCREEN_HEIGHT / transformY));
            int spriteWidth = spriteHeight / 2;

            int drawStartX = spriteScreenX - spriteWidth / 2;
            int drawEndX = spriteScreenX + spriteWidth / 2;
            int drawStartY = -spriteHeight / 2 + SCREEN_HEIGHT / 2;

            // Малюємо ворога, якщо він не перекритий стіною
            if (spriteScreenX >= 0 && spriteScreenX < SCREEN_WIDTH && transformY < zBuffer[spriteScreenX]) {
                // Тіло ворога (білий прямокутник з чорним контуром і очима)
                display.fillRect(drawStartX, drawStartY, spriteWidth, spriteHeight, SSD1306_WHITE);
                display.drawRect(drawStartX, drawStartY, spriteWidth, spriteHeight, SSD1306_BLACK);

                // "Очі" ворога, якщо він достатньо близько
                if (spriteWidth > 6) {
                    int eyeY = drawStartY + spriteHeight / 4;
                    display.fillRect(drawStartX + 2, eyeY, 2, 2, SSD1306_BLACK);
                    display.fillRect(drawEndX - 4, eyeY, 2, 2, SSD1306_BLACK);
                }
            }
        }
    }

    // --- 5. ІНТЕРФЕЙС ТА ЗБРОЯ ---
    // Приціл
    display.drawFastHLine(SCREEN_WIDTH / 2 - 3, SCREEN_HEIGHT / 2, 7, SSD1306_WHITE);
    display.drawFastVLine(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 3, 7, SSD1306_WHITE);

    // Зброя внизу по центру
    int gunOffset = (shootAnim > 0) ? 4 : 0;
    if (shootAnim > 0) {
        // Спалах пострілу
        display.fillCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 14, 5, SSD1306_WHITE);
        shootAnim--;
    }
    display.fillRect(SCREEN_WIDTH / 2 - 4, SCREEN_HEIGHT - 12 + gunOffset, 8, 12, SSD1306_BLACK);
    display.drawRect(SCREEN_WIDTH / 2 - 4, SCREEN_HEIGHT - 12 + gunOffset, 8, 12, SSD1306_WHITE);

    // Показники HP та очок
    display.setCursor(0, 0);
    display.print("HP:");
    display.print(playerHP);
    display.setCursor(80, 0);
    display.print("SC:");
    display.print(score);

    display.display();
}
