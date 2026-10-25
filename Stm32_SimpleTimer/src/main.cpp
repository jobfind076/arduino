#include <Arduino.h>

// Оголошуємо покажчик на об'єкт таймера
HardwareTimer *MyTim;

// Прапорець стану світлодіода
volatile bool ledState = false;

// Функція обробки переривання (Callback / ISR)
// Вона має виконуватися якомога швидше!
void Update_IT_callback(void) 
{
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState);
}

void setup() 
{
  // Налаштовуємо пін вбудованого світлодіода на вихід
  pinMode(LED_BUILTIN, OUTPUT);

  // 1. Ініціалізуємо таймер. Використовуємо TIM2 (доступний майже на всіх STM32)
  // Автоматично вибирається екземпляр для конкретного мікроконтролера
  MyTim = new HardwareTimer(TIM2);

  // 2. Встановлюємо частоту спрацьовування або період.
  // Можна задавати в Герцах (HERTZ_FORMAT) або мікросекундах (MICROSEC_FORMAT).
  // Наприклад, 2 Гц = спрацьовування двічі на секунду (кожні 500 мс)
  MyTim->setOverflow(2, HERTZ_FORMAT);

  // Альтернативний варіант через мікросекунди (раскомментуйте за потреби):
  // MyTim->setOverflow(500000, MICROSEC_FORMAT); 

  // 3. Прив'язуємо нашу функцію обробки переривання до таймера
  MyTim->attachInterrupt(Update_IT_callback);

  // 4. Запускаємо таймер
  MyTim->resume();
}

void loop() 
{
  // Головний цикл абсолютно порожній або виконує іншу логіку.
  // Таймер працює повністю на апаратному рівні.
}
