#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Піни для ESP32-S3
#define I2C_SDA 8
#define I2C_SCL 9
#define BTN_JUMP 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void resetGame();
void drawGameOver();

// Спрайти (16x8 пікселів)
// Місяцехід
const unsigned char PROGMEM rover_bmp[] = {
    0x03, 0x80, // ......###.......
    0x04, 0x40, // .....#...#......
    0x3F, 0xF8, // ..###########...
    0x7F, 0xFC, // .#############..
    0xFF, 0xFE, // ###############.
    0x73, 0x9C, // .###..###..###..
    0x20, 0x08, // ..#.........#...
    0x00, 0x00
};

// Місячний звір (павук/краб)
const unsigned char PROGMEM beast_bmp[] = {
    0x18, 0x18, // ...##......##...
    0x3C, 0x3C, // ..####....####..
    0x7E, 0x7E, // .######..######.
    0xDB, 0xDB, // ##.##.####.##.##
    0xFF, 0xFF, // ################
    0x24, 0x24, // ..#..#....#..#..
    0x42, 0x42, // .#....#..#....#.
    0x81, 0x81 // #......##......#
};

// Космічний птах (кадр 1 - крила вгору)
const unsigned char PROGMEM bird1_bmp[] = {
    0x80, 0x01, // #..............#
    0x40, 0x02, // .#............#.
    0x20, 0x04, // ..#..........#..
    0x13, 0xC8, // ...#..####..#...
    0x0F, 0xF0, // ....########....
    0x01, 0x80, // .......##.......
    0x00, 0x00,
    0x00, 0x00
};

// Космічний птах (кадр 2 - крила вниз)
const unsigned char PROGMEM bird2_bmp[] = {
    0x00, 0x00,
    0x00, 0x00,
    0x01, 0x80, // .......##.......
    0x0F, 0xF0, // ....########....
    0x13, 0xC8, // ...#..####..#...
    0x20, 0x04, // ..#..........#..
    0x40, 0x02, // .#............#.
    0x80, 0x01 // #..............#
};

// Змінні фізики та гри
const int GROUND_Y = 54;
float roverY = GROUND_Y - 8;
float velocityY = 0;
const float gravity = 0.6;
const float jumpForce = -7.5;
bool isGrounded = true;

// Перешкоди
int beastX = 128;
int birdX = 190;
int birdY = 30;
float gameSpeed = 3.0;

// Декор (кратери та зірки)
int craterX[3] = { 20, 70, 115 };
int starX[5] = { 10, 40, 65, 95, 120 };
int starY[5] = { 12, 20, 8, 18, 10 };

int score = 0;
bool gameOver = false;
unsigned long lastFrame = 0;

void setup()
{
    pinMode(BTN_JUMP, INPUT_PULLUP);
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        for (;;)
            ; // Зупинка, якщо дисплей не знайдено
    }
    display.clearDisplay();
}

void resetGame()
{
    roverY = GROUND_Y - 8;
    velocityY = 0;
    beastX = 128;
    birdX = 200;
    gameSpeed = 3.0;
    score = 0;
    gameOver = false;
}

void loop()
{
    // Обмеження FPS (~30 кадрів на секунду)
    if (millis() - lastFrame < 33)
        return;
    lastFrame = millis();

    bool btnPressed = (digitalRead(BTN_JUMP) == LOW);

    if (gameOver) {
        drawGameOver();
        if (btnPressed) {
            delay(200);
            resetGame();
        }
        return;
    }

    // --- ЛОГІКА ГРИ ---
    // Стрибок
    if (btnPressed && isGrounded) {
        velocityY = jumpForce;
        isGrounded = false;
    }

    // Гравітація
    roverY += velocityY;
    velocityY += gravity;

    if (roverY >= GROUND_Y - 8) {
        roverY = GROUND_Y - 8;
        velocityY = 0;
        isGrounded = true;
    }

    // Рух звіра по землі
    beastX -= (int)gameSpeed;
    if (beastX < -16) {
        beastX = 128 + random(10, 60);
        score++;
        gameSpeed += 0.1; // Поступове прискорення
    }

    // Рух птаха у повітрі
    birdX -= (int)(gameSpeed + 1);
    if (birdX < -16) {
        birdX = 128 + random(40, 120);
        birdY = random(24, 42); // Різна висота польоту
        score++;
    }

    // Рух кратерів на поверхні Місяця
    for (int i = 0; i < 3; i++) {
        craterX[i] -= (int)gameSpeed;
        if (craterX[i] < -10)
            craterX[i] = 135;
    }

    // Перевірка зіткнень (хітбокси трохи менші за спрайт для чесності)
    // Зіткнення зі звіром
    if (beastX < 24 && beastX + 12 > 12 && roverY + 6 > GROUND_Y - 8) {
        gameOver = true;
    }
    // Зіткнення з птахом
    if (birdX < 24 && birdX + 12 > 12 && roverY < birdY + 6 && roverY + 6 > birdY) {
        gameOver = true;
    }

    // --- ВІДМАЛЬОВКА ---
    display.clearDisplay();

    // Зірки на фоні
    for (int i = 0; i < 5; i++) {
        display.drawPixel(starX[i], starY[i], SSD1306_WHITE);
    }
    // Місяць/Земля на небі
    display.drawCircle(110, 12, 5, SSD1306_WHITE);

    // Поверхня Місяця
    display.drawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, SSD1306_WHITE);
    for (int i = 0; i < 3; i++) {
        display.drawEllipse(craterX[i], GROUND_Y + 4, 5, 2, SSD1306_WHITE);
    }

    // Місяцехід
    display.drawBitmap(10, (int)roverY, rover_bmp, 16, 8, SSD1306_WHITE);

    // Звір
    display.drawBitmap(beastX, GROUND_Y - 8, beast_bmp, 16, 8, SSD1306_WHITE);

    // Птах (анімація крил кожні 150 мс)
    if ((millis() / 150) % 2 == 0) {
        display.drawBitmap(birdX, birdY, bird1_bmp, 16, 8, SSD1306_WHITE);
    } else {
        display.drawBitmap(birdX, birdY, bird2_bmp, 16, 8, SSD1306_WHITE);
    }

    // Рахунок
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(2, 2);
    display.print("Score: ");
    display.print(score);

    display.display();
}

void drawGameOver()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(35, 20);
    display.println("GAME OVER");
    display.setCursor(38, 35);
    display.print("Score: ");
    display.println(score);
    display.setCursor(18, 50);
    display.println("Press JUMP to start");
    display.display();
}
