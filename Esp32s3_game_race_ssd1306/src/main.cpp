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
#define BTN_LEFT 4
#define BTN_RIGHT 5

// Параметри траси (3 смуги руху)
const int roadLeft = 24;
const int roadRight = 104;
const int laneWidth = (roadRight - roadLeft) / 3; // ~26 пікселів на смугу

// Координати центру для кожної з 3 смуг
const int laneX[3] = {
    roadLeft + laneWidth / 2 - 6,
    roadLeft + laneWidth + laneWidth / 2 - 6,
    roadLeft + 2 * laneWidth + laneWidth / 2 - 6
};

// Параметри машинок
const int carW = 12;
const int carH = 16;

// Змінні гравця
int playerLane = 1; // 0 - ліва, 1 - центральна, 2 - права
const int playerY = SCREEN_HEIGHT - carH - 2;

// Змінні перешкоди (зустрічна машина)
int enemyLane = 0;
float enemyY = -carH;
float gameSpeed = 2.0;

// Стан гри
int score = 0;
bool gameOver = false;
int roadLineOffset = 0;

// Захист від брязкоту контактів
unsigned long lastButtonPress = 0;
const int debounceDelay = 150;

void handleInput();
void updateGame();
void drawGame();
void drawCar(int x, int y, bool isPlayer);
void drawGameOver();
void resetGame();

void setup()
{
    Serial.begin(115200);

    // Налаштування кнопок
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);

    // Ініціалізація I2C для ESP32-S3
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Помилка ініціалізації SSD1306"));
        for (;;)
            ;
    }

    randomSeed(analogRead(1)); // Ініціалізація генератора випадкових чисел
    resetGame();
}

void loop()
{
    if (gameOver) {
        drawGameOver();
        // Перезапуск при натисканні будь-якої кнопки
        if (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
            delay(300);
            resetGame();
        }
        return;
    }

    handleInput();
    updateGame();
    drawGame();

    delay(20); // ~50 FPS
}

// Обробка натискань кнопок
void handleInput()
{
    if (millis() - lastButtonPress > debounceDelay) {
        if (digitalRead(BTN_LEFT) == LOW && playerLane > 0) {
            playerLane--;
            lastButtonPress = millis();
        } else if (digitalRead(BTN_RIGHT) == LOW && playerLane < 2) {
            playerLane++;
            lastButtonPress = millis();
        }
    }
}

// Оновлення фізики та логіки
void updateGame()
{
    // Рух розмітки дороги
    roadLineOffset += (int)gameSpeed;
    if (roadLineOffset >= 16)
        roadLineOffset = 0;

    // Рух ворожої машини
    enemyY += gameSpeed;

    // Якщо ворог проїхав екран
    if (enemyY > SCREEN_HEIGHT) {
        enemyY = -carH;
        enemyLane = random(0, 3);
        score++;

        // Поступове збільшення швидкості
        if (gameSpeed < 6.5) {
            gameSpeed += 0.15;
        }
    }

    // Перевірка зіткнення
    if (playerLane == enemyLane) {
        if (enemyY + carH > playerY + 2 && enemyY < playerY + carH - 2) {
            gameOver = true;
        }
    }
}

// Малювання кадру
void drawGame()
{
    display.clearDisplay();

    // Межі траси
    display.drawFastVLine(roadLeft, 0, SCREEN_HEIGHT, SSD1306_WHITE);
    display.drawFastVLine(roadRight, 0, SCREEN_HEIGHT, SSD1306_WHITE);

    // Пунктирна розмітка смуг
    for (int y = -16 + roadLineOffset; y < SCREEN_HEIGHT; y += 16) {
        display.drawFastVLine(roadLeft + laneWidth, y, 8, SSD1306_WHITE);
        display.drawFastVLine(roadLeft + 2 * laneWidth, y, 8, SSD1306_WHITE);
    }

    // Малюємо машину гравця
    drawCar(laneX[playerLane], playerY, true);

    // Малюємо зустрічну машину
    drawCar(laneX[enemyLane], (int)enemyY, false);

    // Вивід рахунку зліва від траси
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 5);
    display.println("PTS");
    display.setCursor(0, 16);
    display.println(score);

    display.display();
}

// Функція малювання деталізованої машинки
void drawCar(int x, int y, bool isPlayer)
{
    // Кузов
    display.drawRoundRect(x + 2, y, carW - 4, carH, 2, SSD1306_WHITE);

    if (isPlayer) {
        // Лобове скло гравця (спереду)
        display.fillRect(x + 4, y + 3, carW - 8, 4, SSD1306_WHITE);
    } else {
        // Лобове скло зустрічної машини (знизу)
        display.fillRect(x + 4, y + carH - 7, carW - 8, 4, SSD1306_WHITE);
    }

    // Колеса
    display.fillRect(x, y + 2, 2, 4, SSD1306_WHITE);
    display.fillRect(x + carW - 2, y + 2, 2, 4, SSD1306_WHITE);
    display.fillRect(x, y + carH - 6, 2, 4, SSD1306_WHITE);
    display.fillRect(x + carW - 2, y + carH - 6, 2, 4, SSD1306_WHITE);
}

// Екран програшу
void drawGameOver()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 12);
    display.println("GAME OVER");

    display.setTextSize(1);
    display.setCursor(32, 36);
    display.print("Score: ");
    display.println(score);

    display.setCursor(14, 50);
    display.println("Press any button");
    display.display();
}

// Скидання параметрів для нової гри
void resetGame()
{
    playerLane = 1;
    enemyLane = random(0, 3);
    enemyY = -carH;
    score = 0;
    gameSpeed = 2.0;
    gameOver = false;
}
