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
#define BALL_SIZE 1       // размер шара
#define RANDOM_COLOR 1    // случайный цвет при отскоке

// эффект "огонь"
#define SPARKLES 1        // вылетающие угольки вкл выкл
#define HUE_ADD 0         // добавка цвета в огонь (от 0 до 230) - меняет весь цвет пламени

// эффект "кометы"
#define TAIL_STEP 30      // длина хвоста кометы
#define SATURATION 150    // насыщенность кометы (от 0 до 255)

byte hue;
bool loadingFlag=true;

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

void colorsRoutine() {
  hue += 4;
  for (int i = 0; i < NUMPIXELS; i++) {
    if (getPixColor(i) > 0) pixels.setPixelColorHsv(i, hue, 255, 255);
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


void matrixRoutine() {
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

int coordB[2];
int8_t vectorB[2];
uint32_t ballColor;

void ballRoutine() {
  bool hit=false;
  if (loadingFlag) {
    for (byte i = 0; i < 2; i++) {
      coordB[i] = WIDTH / 2 * 10;
      vectorB[i] = random(8, 20);
      ballColor = pixels.ColorHsv(random(0, 9) * 28, 255, 255);
    }
    loadingFlag = false;
  }
  for (byte i = 0; i < 2; i++) {
    coordB[i] += vectorB[i];
    if (coordB[i] < 0) {
      hit=true;
      coordB[i] = 0;
      vectorB[i] = -vectorB[i];
      vectorB[i] += random(0, 6) - 3;
    }
  }
  if (coordB[0] >= (WIDTH - BALL_SIZE) * 10) {
    hit=true;
    coordB[0] = (WIDTH - BALL_SIZE) * 10;
    vectorB[0] = -vectorB[0];
    vectorB[0] += random(0, 6) - 3;
  }
  if (coordB[1] >= (HEIGHT - BALL_SIZE) * 10) {
    hit=true;
    coordB[1] = (HEIGHT - BALL_SIZE) * 10;
    vectorB[1] = -vectorB[1];
    vectorB[1] += random(0, 6) - 3;
  }
  if (RANDOM_COLOR && hit) ballColor = pixels.ColorHsv(random(0, 9) * 28, 255, 255);
  pixels.clear();
  for (byte i = 0; i < BALL_SIZE; i++)
    for (byte j = 0; j < BALL_SIZE; j++)
      pixels.setPixelColor(getPixelNumber(coordB[0] / 10 + i, coordB[1] / 10 + j), ballColor);
}


// *********** снегопад 2.0 ***********
void snowRoutine() {
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

void rainbowRoutine() {

  hue += 3;
  for (byte i = 0; i < WIDTH; i++) {
    uint32_t thisColor = pixels.ColorHsv((byte)(hue + i * float(255 / WIDTH)), 255, 255);
    for (byte j = 0; j < HEIGHT; j++)
      drawPixelXY(i, j, thisColor);
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
  if(t-te > 250) te=t, ballRoutine();
  pixels.show(); // This sends the updated pixel color to the hardware.
}

