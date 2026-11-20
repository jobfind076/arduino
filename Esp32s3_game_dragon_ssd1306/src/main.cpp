#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// Налаштування дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Піни для ESP32-S3
#define I2C_SDA 8
#define I2C_SCL 9
#define JUMP_BTN 0 // Вбудована кнопка BOOT на платі ESP32-S3

// Параметри фізики та землі
const int GROUND_Y = 54;
const float GRAVITY = 0.65;
const float JUMP_FORCE = -7.5;

// Параметри дракончика
const int DINO_X = 16;
const int DINO_W = 12;
const int DINO_H = 14;
float dinoY = GROUND_Y - DINO_H;
float dinoVelocityY = 0;
bool isGrounded = true;

// Параметри перешкод (кактусів)
int obsX = SCREEN_WIDTH;
int obsW = 6;
int obsH = 12;
float gameSpeed = 3.5;

// Стан гри
int score = 0;
bool gameOver = false;

// Спрайт дракончика (12x14 пікселів)
const unsigned char PROGMEM dinoBitmap[] = {
    0x07, 0xE0, //      ######
    0x0F, 0xF0, //     ########
    0x0D, 0xF0, //     ## #####
    0x0F, 0xF0, //     ########
    0x0F, 0x00, //     ####
    0x0F, 0xC0, //     ######
    0x9F, 0x00, // #  #####
    0xDF, 0x80, // ## ######
    0xFF, 0x00, // ########
    0x7F, 0x00, //  #######
    0x3E, 0x00, //   #####
    0x1C, 0x00, //    ###
    0x14, 0x00, //    # #
    0x36, 0x00 //   ## ##
};

void resetGame();
void drawGameOver();

void setup()
{
    pinMode(JUMP_BTN, INPUT_PULLUP);

    // Ініціалізація I2C на пінах ESP32-S3
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        for (;;)
            ; // Зупинка, якщо дисплей не знайдено
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
}

void loop()
{
    bool btnPressed = (digitalRead(JUMP_BTN) == LOW);

    if (gameOver) {
        drawGameOver();
        if (btnPressed) {
            resetGame();
            delay(250); // Захист від випадкового стрибка при рестарті
        }
        return;
    }

    // --- 1. КЕРУВАННЯ ТА ФІЗИКА ---
    if (btnPressed && isGrounded) {
        dinoVelocityY = JUMP_FORCE;
        isGrounded = false;
    }

    dinoY += dinoVelocityY;
    dinoVelocityY += GRAVITY;

    // Перевірка приземлення
    if (dinoY >= GROUND_Y - DINO_H) {
        dinoY = GROUND_Y - DINO_H;
        dinoVelocityY = 0;
        isGrounded = true;
    }

    // --- 2. РУХ ПЕРЕШКОД ---
    obsX -= (int)gameSpeed;
    if (obsX < -obsW) {
        obsX = SCREEN_WIDTH + random(10, 45); // Нова перешкода з випадковою затримкою
        obsH = random(10, 17); // Випадкова висота кактуса
        score++;
        gameSpeed += 0.12; // Поступове прискорення гри
    }

    // --- 3. ЛОГІКА КОЛІЗІЙ (AABB з пробаченням країв) ---
    // Зменшуємо хітбокс на 2 пікселі з кожного боку, щоб гра була чеснішою
    int hitboxMargin = 2;

    int dinoLeft = DINO_X + hitboxMargin;
    int dinoRight = DINO_X + DINO_W - hitboxMargin;
    int dinoTop = (int)dinoY + hitboxMargin;
    int dinoBottom = (int)dinoY + DINO_H - hitboxMargin;

    int obsLeft = obsX + 1;
    int obsRight = obsX + obsW - 1;
    int obsTop = GROUND_Y - obsH + 1;
    int obsBottom = GROUND_Y;

    // Перевірка перетину двох прямокутників
    if (dinoRight > obsLeft && dinoLeft < obsRight && dinoBottom > obsTop && dinoTop < obsBottom) {
        gameOver = true;
    }

    // --- 4. ВІДМАЛЬОВКА КАДРУ ---
    display.clearDisplay();

    // Лінія землі
    display.drawFastHLine(0, GROUND_Y, SCREEN_WIDTH, SSD1306_WHITE);

    // Дракончик
    display.drawBitmap(DINO_X, (int)dinoY, dinoBitmap, DINO_W, DINO_H, SSD1306_WHITE);

    // Перешкода (заокруглений прямокутник у вигляді кактуса)
    display.fillRoundRect(obsX, GROUND_Y - obsH, obsW, obsH, 2, SSD1306_WHITE);

    // Рахунок
    display.setTextSize(1);
    display.setCursor(90, 4);
    display.print("SC:");
    display.print(score);

    display.display();
    delay(20); // ~50 FPS
}

void resetGame()
{
    dinoY = GROUND_Y - DINO_H;
    dinoVelocityY = 0;
    isGrounded = true;
    obsX = SCREEN_WIDTH;
    score = 0;
    gameSpeed = 3.5;
    gameOver = false;
}

void drawGameOver()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 15);
    display.println("GAME OVER");

    display.setTextSize(1);
    display.setCursor(35, 38);
    display.print("Score: ");
    display.println(score);

    display.setCursor(18, 52);
    display.print("Press BOOT to play");
    display.display();
}
