#include <Arduino.h>

// Використовуємо світлодіод, пін 2
#define LED_PIN 2

// Змінна для зберігання структури таймера
hw_timer_t * timer = NULL;

// Змінна для відстеження стану світлодіода (volatile, бо змінюється в перериванні)
volatile bool ledState = false;

// Обробник переривання (ISR) - виконується в IRAM для швидкої роботи
void IRAM_ATTR onTimer() 
{
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
}

void setup() 
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  /* 
    1. timerBegin(номер_таймера, дільник, рахувати_вгору)
    - Номер таймера: 0 (доступно від 0 до 3)
    - Дільник (Prescaler): 80. Базова частота шини APB становить 80 МГц.
      80 000 000 Гц / 80 = 1 000 000 тіків на секунду (1 тік = 1 мікросекунда).
    - true: лічильник рахує вгору.
  */
  timer = timerBegin(0, 80, true);

  /* 
    2. timerAttachInterrupt(таймер, обробник, тип_спрацьовування)
    - Прив'язуємо нашу функцію onTimer до створеного таймера.
    - true: генерувати переривання по фронту (edge).
  */
  timerAttachInterrupt(timer, &onTimer, true);

  /* 
    3. timerAlarmWrite(таймер, кількість_тіків, автоперезапуск)
    - Оскільки частота 1 000 000 тіків/сек, значення 1000000 означає рівно 1 секунду.
    - true: таймер автоматично перезапускається після спрацьовування (циклічний).
  */
  timerAlarmWrite(timer, 1000000, true);

  /* 
    4. timerAlarmEnable(таймер)
    - Активуємо будильник (alarm) таймера.
  */
  timerAlarmEnable(timer);

  Serial.println("Таймер успішно запущено за старим API!");
}

void loop() 
{
  // Основний цикл залишається вільним
  delay(1000); 
}