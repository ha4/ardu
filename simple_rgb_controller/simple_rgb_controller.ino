/*
    Прошивка к простому RGB контроллеру,
    управляемому 2мя аналоговыми потенциометрами.
    МК - ATtiny85(45)
    Clock - 4 MHz (Internal)
*/

//#define COLOR_HUE_LUM
//#define COLOR_XY
#define COLOR_STICK

#include <util/delay.h>

/* Указываем пины ШИМ и АЦП */
#define R_PWM_PIN   1
#define G_PWM_PIN   0
#define B_PWM_PIN   4
#define COLOR_PIN   A3
#define BRIGHT_PIN  A1

uint8_t r_pwm = 0, g_pwm = 0, b_pwm = 0;          // Компоненты RGB

void setup() {
  pinMode(R_PWM_PIN, OUTPUT);                     // Настраиваем пины с ШИМ на выход
  pinMode(G_PWM_PIN, OUTPUT);
  pinMode(B_PWM_PIN, OUTPUT);

  TCCR0B = (TCCR0B & 0b11111000) | 1;             // ШИМ 16 кгц для таймера 0
  TCCR1 = (TCCR1 & 0b11110000) | 1;               // ШИМ 16 кгц для таймера 1
}

void loop() {
  /* Получаем аналоговые значения с потенциометров */
  uint8_t a = analogRead(COLOR_PIN) >> 2;     // Для этого удаляем младшие 2 бита
  uint8_t b = analogRead(BRIGHT_PIN) >> 2;   // Получаем 8-ми битные значения
  uint8_t bright;

  /* Получаем RGB, корректируем и применяем яркость */
#if defined(COLOR_HUE_LUM)
  makeColor(a);                               // Преобразуем оттенок в RGB компоненты
  bright=b;
#elif defined(COLOR_XY)
  makeColorXY(a,b);
  bright=255;
#elif defined(COLOR_STICK)
  bright=makeColorSTK(a,b);
#endif
  bright = getCRT(bright);                        // Пропускаем яркость через CRT
  makeBright(bright);                             // Применяем яркость на компоненты RGB

  /* Выводим RGB на LED драйвер в виде ШИМ сигнала */
  analogWrite(R_PWM_PIN, r_pwm);                  // Генерируем ШИМ сигналы
  analogWrite(G_PWM_PIN, g_pwm);
  analogWrite(B_PWM_PIN, b_pwm);

  _delay_ms(20);                                  // Софт-delay, независимый от таймера 0
}

void makeColor(uint8_t color) {                   // 8-ми битное цветовое колесо
  uint8_t shift = 0;                              // Цветовой сдвиг
  if (color > 170) {                              // Синий - фиолетовый - красный
    shift = (color - 170) * 3;                    // Получаем цветовой сдвиг 0 - 255
    r_pwm = shift, g_pwm = 0, b_pwm = ~shift;     // Получаем компоненты RGB
  } else if (color > 85) {                        // Зеленый - голубой - синий
    shift = (color - 85) * 3;                     // Получаем цветовой сдвиг 0 - 255
    r_pwm = 0, g_pwm = ~shift, b_pwm = shift;     // Получаем компоненты RGB
  } else {                                        // Красный - оранжевый - зеленый
    shift = (color - 0) * 3;                      // Получаем цветовой сдвиг 0 - 255
    r_pwm = ~shift, g_pwm = shift, b_pwm = 0;     // Получаем компоненты RGB
  }
}
void makeColorXY(uint8_t x, uint8_t y) {
    uint16_t r,g,b,mx;
    r=x;
    g=255-x;
    b=y;
    mx=r; if(g>mx)mx=g;
    r=(r*255)/mx;
    g=(g*255)/mx;
    if(b>mx)mx=b;
    b=(b*255)/mx;
    r_pwm = r;
    g_pwm = g;
    b_pwm = b;
}

uint8_t sat255(uint8_t x, int8_t i)
{
  int16_t d;
  d=x;
  d+=i;
  if (d<0) d=0; else if(d>255)d=255;
  return d;
}

#define TOL 12 /* zero tolerance, 10% */
#define CTL_COUNTDOWN 4
#define CTL_SCALE 4
uint8_t makeColorSTK(uint8_t y, uint8_t x)
{
    static uint8_t r=128,g=128,b=128,v=255;
    static uint8_t cnt=0,ch=0;
    static int8_t ia, ib, pb=0;

    ia = (int16_t)y-(int16_t)128;
    ib = (int16_t)x-(int16_t)128;
    if(ia>-TOL && ia<TOL) ia=0; else ia=(ia<0)?ia+TOL:ia-TOL;
    if(ib>-TOL && ib<TOL) ib=0; else ib=(ib<0)?ib+TOL:ib-TOL;
    ib = -ib;
    ib /= CTL_SCALE;
    ia /= CTL_SCALE;

    if (ib < 0 && pb==0) ch--;
    if (ib > 0 && pb==0) ch++;
    pb = ib;
    ch &= 3;

    if (ia!=0 && !cnt) {
      cnt = CTL_COUNTDOWN;
      switch(ch) {
      case 0: v = sat255(v,ia); break;
      case 1: r = sat255(r,ia); break;
      case 2: g = sat255(g,ia); break;
      case 3: b = sat255(b,ia); break;
      }
    } else if (ia!=0) cnt--;
    r_pwm = r;
    g_pwm = g;
    b_pwm = b;
    return v;
}

void makeBright(uint8_t brig) {                   // Применяем яркость на компоненты RGB
  r_pwm = ((uint16_t)r_pwm  * (brig + 1)) >> 8;   // Применяем яркость
  g_pwm = ((uint16_t)g_pwm  * (brig + 1)) >> 8;   // Применяем яркость
  b_pwm = ((uint16_t)b_pwm  * (brig + 1)) >> 8;   // Применяем яркость
}

uint8_t getCRT(uint8_t val) {                     // Кубическая CRT гамма
  if (!val) return 0;                             // Принудительный 0
  return ((long)val * val * val + 130305) >> 16;  // Рассчитываем, используя полином
}
