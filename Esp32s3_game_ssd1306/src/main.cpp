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
#define POT_PIN 4 // Пін для потенціометра керування (10K, лівий - +3.3, центр - gpoi4, правий - GND)

// Параметри ракеток
const int PADDLE_WIDTH = 3;
const int PADDLE_HEIGHT = 16;
const int PLAYER_X = 4;
const int AI_X = SCREEN_WIDTH - 4 - PADDLE_WIDTH;

int playerY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
int aiY = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
int aiSpeed = 2; // Швидкість реакції бота

// Параметри м'яча (використовуємо float для плавного руху та кутів)
float ballX = SCREEN_WIDTH / 2;
float ballY = SCREEN_HEIGHT / 2;
float ballSpeedX = 2.2;
float ballSpeedY = 1.5;
const int BALL_SIZE = 3;

// Рахунок
int playerScore = 0;
int aiScore = 0;

void updateControls();
void updateAI();
void updateBallAndCollisions();
void resetBall(int direction);
void drawGame();

void setup()
{
    Serial.begin(115200);

    // Ініціалізація I2C на вказаних пінах ESP32-S3
    Wire.begin(I2C_SDA, I2C_SCL);

    // Ініціалізація дисплея (адреса зазвичай 0x3C)
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Помилка ініціалізації SSD1306"));
        for (;;)
            ;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Налаштування роздільної здатності АЦП для ESP32-S3 (0 - 4095)
    analogReadResolution(12);
}

void loop()
{
    updateControls();
    updateAI();
    updateBallAndCollisions();
    drawGame();

    delay(16); // ~60 FPS
}

// Зчитування позиції ракетки гравця з потенціометра
void updateControls()
{
    int potValue = analogRead(POT_PIN);
    // Перетворюємо значення АЦП (0..4095) у координату Y на екрані
    playerY = map(potValue, 0, 4095, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
}

// Проста логіка руху суперника
void updateAI()
{
    int aiCenter = aiY + PADDLE_HEIGHT / 2;

    // Бот рухається лише коли м'яч летить у його бік
    if (ballSpeedX > 0) {
        if (aiCenter < ballY - 2) {
            aiY += aiSpeed;
        } else if (aiCenter > ballY + 2) {
            aiY -= aiSpeed;
        }
    }

    // Обмеження виходу ракетки бота за межі екрана
    aiY = constrain(aiY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
}

// Головна логіка руху м'яча та обробка колізій
void updateBallAndCollisions()
{
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    // 1. Колізія з верхньою та нижньою межами екрана
    if (ballY <= 0) {
        ballY = 0;
        ballSpeedY = -ballSpeedY;
    } else if (ballY + BALL_SIZE >= SCREEN_HEIGHT) {
        ballY = SCREEN_HEIGHT - BALL_SIZE;
        ballSpeedY = -ballSpeedY;
    }

    // 2. Колізія з ракеткою гравця (AABB - перевірка перетину прямокутників)
    if (ballSpeedX < 0 && ballX <= PLAYER_X + PADDLE_WIDTH && ballX + BALL_SIZE >= PLAYER_X && ballY + BALL_SIZE >= playerY && ballY <= playerY + PADDLE_HEIGHT) {

        ballX = PLAYER_X + PADDLE_WIDTH; // Запобігаємо "застряганню" м'яча в ракетці
        ballSpeedX = -ballSpeedX * 1.05; // Відбиття + легке прискорення

        // Зміна кута відскоку залежно від того, якою частиною ракетки відбили
        float hitOffset = (ballY + BALL_SIZE / 2.0) - (playerY + PADDLE_HEIGHT / 2.0);
        ballSpeedY = hitOffset * 0.25;
    }

    // 3. Колізія з ракеткою суперника (ШІ)
    if (ballSpeedX > 0 && ballX + BALL_SIZE >= AI_X && ballX <= AI_X + PADDLE_WIDTH && ballY + BALL_SIZE >= aiY && ballY <= aiY + PADDLE_HEIGHT) {

        ballX = AI_X - BALL_SIZE;
        ballSpeedX = -ballSpeedX * 1.05;

        float hitOffset = (ballY + BALL_SIZE / 2.0) - (aiY + PADDLE_HEIGHT / 2.0);
        ballSpeedY = hitOffset * 0.25;
    }

    // Обмеження максимальної швидкості, щоб м'яч не "пролітав" крізь ракетки
    ballSpeedX = constrain(ballSpeedX, -5.0, 5.0);

    // 4. Перевірка голів (вихід за ліву або праву межу)
    if (ballX < 0) {
        aiScore++;
        resetBall(1); // Подача в бік гравця
    } else if (ballX > SCREEN_WIDTH) {
        playerScore++;
        resetBall(-1); // Подача в бік бота
    }
}

// Скидання м'яча в центр після голу
void resetBall(int direction)
{
    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    ballSpeedX = 2.2 * direction;
    ballSpeedY = random(-15, 15) / 10.0; // Випадковий початковий кут
    delay(500); // Коротка пауза перед подачею
}

// Відмальовування кадру
void drawGame()
{
    display.clearDisplay();

    // Центральна пунктирна лінія
    for (int i = 0; i < SCREEN_HEIGHT; i += 6) {
        display.drawFastVLine(SCREEN_WIDTH / 2, i, 3, SSD1306_WHITE);
    }

    // Рахунок
    display.setCursor(SCREEN_WIDTH / 2 - 18, 4);
    display.print(playerScore);
    display.setCursor(SCREEN_WIDTH / 2 + 12, 4);
    display.print(aiScore);

    // Ракетки та м'яч
    display.fillRect(PLAYER_X, playerY, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
    display.fillRect(AI_X, aiY, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
    display.fillRect((int)ballX, (int)ballY, BALL_SIZE, BALL_SIZE, SSD1306_WHITE);

    display.display();
}
