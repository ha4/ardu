// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            11

// How many NeoPixels are attached to the Arduino?
#define WIDTH 6
#define HEIGHT 5
#define NUMPIXELS      (WIDTH*HEIGHT)
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define getPixelNumber(x,y) ((x)*HEIGHT+(HEIGHT-1-(y)))
#define getPixColor(i)     (pixels.getPixelColor(i))
#define getPixColorXY(x,y) (pixels.getPixelColor(getPixelNumber(x,y)))
#define drawPixelXY(x,y,c) (pixels.setPixelColor(getPixelNumber(x,y),c))

#define WAVES_AMOUNT 2    // количество синусоид

// эффект "шарики"
#define BALLS_AMOUNT 3    // количество "шариков"
#define CLEAR_PATH 1      // очищать путь
#define BALL_TRACK 1      // (0 / 1) - вкл/выкл следы шариков
#define DRAW_WALLS 1      // режим с рисованием препятствий для шаров
#define TRACK_STEP 40     // длина хвоста шарика (чем больше цифра, тем хвост короче)

// эффект "квадратик"
#define BALL_SIZE 3       // размер шара
#define RANDOM_COLOR 1    // случайный цвет при отскоке

// эффект "огонь"
#define SPARKLES 1        // вылетающие угольки вкл выкл
#define HUE_ADD 0         // добавка цвета в огонь (от 0 до 230) - меняет весь цвет пламени

// эффект "кометы"
#define TAIL_STEP 30      // длина хвоста кометы
#define SATURATION 150    // насыщенность кометы (от 0 до 255)


void fadePixel(byte i, byte j, byte step) {
  uint32_t thisColor = getPixColorXY(i, j);
  if (thisColor == 0) return;

  byte rgb[3];
  rgb[0] = (thisColor >> 16) & 0xff;
  rgb[1] = (thisColor >> 8) & 0xff;
  rgb[2] = thisColor & 0xff;

  byte maximum = max(max(rgb[0], rgb[1]), rgb[2]);
  float coef = 0;

  if (maximum >= step)
    coef = (float)(maximum - step) / maximum;
  for (byte i = 0; i < 3; i++) {
    if (rgb[i] > 0) rgb[i] = (float)rgb[i] * coef;
    else rgb[i] = 0;
  }
  pixels.setPixelColor(getPixelNumber(i, j), pixels.Color(rgb[0], rgb[1], rgb[2]));
}

// функция плавного угасания цвета для всех пикселей
void fader() {
  for (byte i = 0; i < WIDTH; i++) {
    for (byte j = 0; j < HEIGHT; j++) {
      fadePixel(i, j, TRACK_STEP);
    }
  }
}

void effect1()
{
    for (byte i = HEIGHT / 2; i < HEIGHT; i++) {
      if (getPixColorXY(0, i) == 0
          && (random(0, 60) == 0)
          && getPixColorXY(0, i + 1) == 0
          && getPixColorXY(0, i - 1) == 0)
        pixels.setPixelColorHsv(getPixelNumber(0, i), random(0, 200), SATURATION, 255);
    }
    for (byte i = 0; i < WIDTH / 2; i++) {
      if (getPixColorXY(i, HEIGHT - 1) == 0
          && (random(0, 60) == 0)
          && getPixColorXY(i + 1, HEIGHT - 1) == 0
          && getPixColorXY(i - 1, HEIGHT - 1) == 0)
        pixels.setPixelColorHsv(getPixelNumber(i, HEIGHT - 1), random(0, 200), SATURATION, 255);
    }

    // сдвигаем по диагонали
    for (byte y = 0; y < HEIGHT - 1; y++) {
      for (byte x = WIDTH - 1; x > 0; x--) {
        drawPixelXY(x, y, getPixColorXY(x - 1, y + 1));
      }
    }

    // уменьшаем яркость левой и верхней линии, формируем "хвосты"
    for (byte i = HEIGHT / 2; i < HEIGHT; i++) {
      fadePixel(0, i, TRACK_STEP);
    }
    for (byte i = 0; i < WIDTH / 2; i++) {
      fadePixel(i, HEIGHT - 1, TAIL_STEP);
    }
}


void effect2() {
    for (byte x = 0; x < WIDTH; x++) {
      // заполняем случайно верхнюю строку
      uint32_t thisColor = getPixColorXY(x, HEIGHT - 1);
      if (thisColor == 0)
        drawPixelXY(x, HEIGHT - 1, 0x00FF00 * (random(0, 10) == 0));
      else if (thisColor < 0x002000)
        drawPixelXY(x, HEIGHT - 1, 0);
      else
        drawPixelXY(x, HEIGHT - 1, thisColor - 0x002000);
    }

    // сдвигаем всё вниз
    for (byte x = 0; x < WIDTH; x++) {
      for (byte y = 0; y < HEIGHT - 1; y++) {
        drawPixelXY(x, y, getPixColorXY(x, y + 1));
      }
    }
}

byte hue;

void colorsRoutine() {
    hue += 2;
    for (int i = 0; i < NUMPIXELS; i++) {
      if (getPixColor(i) > 0) pixels.setPixelColorHsv(i, hue, 255, 255);
    }
}

// *********** снегопад 2.0 ***********
void effect() {
    // сдвигаем всё вниз
    for (byte x = 0; x < WIDTH; x++) {
      for (byte y = 0; y < HEIGHT - 1; y++) {
        drawPixelXY(x, y, getPixColorXY(x, y + 1));
      }
    }

    for (byte x = 0; x < WIDTH; x++) {
      // заполняем случайно верхнюю строку
      // а также не даём двум блокам по вертикали вместе быть
      if (getPixColorXY(x, HEIGHT - 2) == 0 && (random(0, 10) == 0))
        drawPixelXY(x, HEIGHT - 1, 0xE0FFFF - 0x101010 * random(0, 4));
      else
        drawPixelXY(x, HEIGHT - 1, 0x000000);
    }
}

void effect3() {
    hue++;
    for (byte i = 0; i < WIDTH; i++) {
//      CHSV thisColor = CHSV((byte)(hue + i * float(255 / WIDTH)), 255, 255);
//      for (byte j = 0; j < HEIGHT; j++)
//        if (getPixColor(getPixelNumber(i, j)) > 0) leds[getPixelNumber(i, j)] = thisColor;
    }
}

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  pixels.begin(); // This initializes the NeoPixel library.
}

uint32_t te=0;
void loop() {
  uint32_t t=millis();
  if(t-te > 50) te=t, effect();
  pixels.show(); // This sends the updated pixel color to the hardware.
}

#if 0

  void setPixelColorHsv(uint16_t n, uint16_t h, uint8_t s, uint8_t v);


// Set pixel color from HSV colorspace:
// See http://www.vagrearg.org/content/hsvrgb for in depth algorithm and
// implementation explanation.
void Adafruit_NeoPixel::setPixelColorHsv(
 uint16_t n, uint16_t h, uint8_t s, uint8_t v) {
  if(n >= numLEDs)
    return;
  uint8_t r, g, b;
  if(!s) {
    // Monochromatic, all components are V
    r = g = b = v;
  } else {
    uint8_t sextant = h >> 8;
    if(sextant > 5)
      sextant = 5;  // Limit hue sextants to defined space
    g = v;    // Top level
    // Perform actual calculations
    /*
     * Bottom level:
     * --> (v * (255 - s) + error_corr + 1) / 256
     */
    uint16_t ww;        // Intermediate result
    ww = v * (uint8_t)(~s);
    ww += 1;            // Error correction
    ww += ww >> 8;      // Error correction
    b = ww >> 8;
    uint8_t h_fraction = h & 0xff;  // Position within sextant
    uint32_t d;      // Intermediate result
    if(!(sextant & 1)) {
      // r = ...slope_up...
      // --> r = (v * ((255 << 8) - s * (256 - h)) + error_corr1 + error_corr2) / 65536
      d = v * (uint32_t)(0xff00 - (uint16_t)(s * (256 - h_fraction)));
      d += d >> 8;  // Error correction
      d += v;       // Error correction
      r = d >> 16;
    } else {
      // r = ...slope_down...
      // --> r = (v * ((255 << 8) - s * h) + error_corr1 + error_corr2) / 65536
      d = v * (uint32_t)(0xff00 - (uint16_t)(s * h_fraction));
      d += d >> 8;  // Error correction
      d += v;       // Error correction
      r = d >> 16;
    }
    // Swap RGB values according to sextant. This is done in reverse order with
    // respect to the original because the swaps are done after the
    // assignments.
    if(!(sextant & 6)) {
      if(!(sextant & 1)) {
        uint8_t tmp = r;
        r = g;
        g = tmp;
      }
    } else {
      if(sextant & 1) {
        uint8_t tmp = r;
        r = g;
        g = tmp;
      }
    }
    if(sextant & 4) {
      uint8_t tmp = g;
      g = b;
      b = tmp;
    }
    if(sextant & 2) {
      uint8_t tmp = r;
      r = b;
      b = tmp;
    }
  }
  // At this point, RGB values are assigned.
  if(brightness) { // See notes in setBrightness()
    r = (r * brightness) >> 8;
    g = (g * brightness) >> 8;
    b = (b * brightness) >> 8;
  }
  uint8_t *p;
  if(wOffset == rOffset) { // Is an RGB-type strip
    p = &pixels[n * 3];    // 3 bytes per pixel
  } else {                 // Is a WRGB-type strip
    p = &pixels[n * 4];    // 4 bytes per pixel
    p[wOffset] = 0;        // But only R,G,B passed -- set W to 0
  }
  p[rOffset] = r;          // R,G,B always stored
  p[gOffset] = g;
  p[bOffset] = b;
}
#endif

