// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#if defined(__AVR_ATtiny85__)
#define PIN            1
#else
#define PIN            5
#endif

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      42
#define SUBPIXELS      4
#define SHSPIX         2
#define NUMRAMP        11 /* 11 or 9, rest < 4 */
#define PIXMAX         255
#define PIXSTEP        ((int)PIXMAX/NUMRAMP)
#define PIXBASE        (PIXMAX-(NUMRAMP)*(PIXSTEP))

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 23; // delay for half a second
int pix  = 0;
int spix = 0;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  pixels.begin(); // This initializes the NeoPixel library.
}

void loop()
{
  int c0 = 0;
  int c1 = PIXBASE;
  for(int i=0;i<=NUMRAMP;i++){ // one transition pixel
    if (i==NUMRAMP) c1=0;
    int a = i+pix; if (a>=NUMPIXELS) a-=NUMPIXELS;
    int c = (c1*(SUBPIXELS-spix) + (c0*spix)) >> SHSPIX;
    pixels.setPixelColor(a, pixels.Color(c/10,c,c/4));
    c0 = c1;
    c1 += PIXSTEP;
  }

  pixels.show(); // This sends the updated pixel color to the hardware.

  delay(delayval); // Delay for a period of time (in milliseconds).
  spix++;
  if (spix >= SUBPIXELS) {
      spix=0;
      pix++;
      if (pix >= NUMPIXELS) pix=0;
  }
}
