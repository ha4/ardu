
#include <LiquidCrystal.h>
#include <AD770X.h>
#include <OneWire.h>

// connection PINOUT
enum { sp_mosi = 11, sp_miso = 12, sp_sck = 13 };
enum { led = 13, analog_a=A7, analog_b=A6, mclk=9, cs_7705=10};
enum { wire1 = 15 }; // on pin PC1
enum { lcd_rs=7, lcd_e=6, lcd_d0=5, lcd_d1=4, lcd_d2=3, lcd_d3=2 };

enum { samples = 32 };

float refmVolts = 1087.14;
boolean go=0;

OneWire  bus1(wire1);  
AD770X ad7705(refmVolts, cs_7705);
LiquidCrystal lcd(lcd_rs, lcd_e, lcd_d0, lcd_d1, lcd_d2, lcd_d3);


void setup() {
//  pinMode(led, OUTPUT);

  Serial.begin(9600); 

  analogReference(INTERNAL);
  analogRead(analog_a);

  lcd.begin(16, 2);

  pinMode(mclk, OUTPUT); // start clocking
  initTimer1();

  ad7705.reset();
  ad7705.init(AD770X::CHN_AIN1, AD770X::BIPOLAR, AD770X::GAIN_1, AD770X::CLK_DIV_2, AD770X::UPDATE_RATE_20);
  
  init_zs(AD770X::CHN_AIN1);
}

void  init_zs(byte chan) {
  ad7705.command(AD770X::REG_SETUP|chan|AD770X::REG_WR,
                 AD770X::MODE_ZERO_SCALE_CAL|AD770X::BIPOLAR|AD770X::GAIN_1);
  ad7705.wait(chan);
}

void initTimer1() {  // PB1, OC1A, D9 pin 2MHz clock
                     // CTC mode 0100, OCR1A=3 ( /4)
                     // toggle COM1A mode = 01 ( /2)
                     // clock src 001 = clk/1   ( 16Mhz)
TCCR1A = (0 << COM1A1)| (1 << COM1A0)| (0 << WGM11)| (0 << WGM10);
TCCR1B = (0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11) |(1 << CS10);
OCR1AL = 3;
OCR1AH = 0;
} 

void print_byte(byte h)
{
    if (h < 16) Serial.print("0");
    Serial.print(h, HEX);
}


int ds18b20(boolean pser, boolean plcd)
{
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  float temp;

  // discover cycle  
  if (bus1.reset()) {
     bus1.write(0xf0);
     if ( !bus1.discover(addr)) {
         if (pser) Serial.print("No more addresses.\n");
         if (plcd) lcd.print("\n");
         bus1.start();
         return 0;
     }
  } else {
         if (pser) Serial.print("No devices.\n");
         return 0;
  }
  
  if (pser) {
    Serial.print("R=");
    for( i = 0; i < 8; i++)
        print_byte(addr[i]);
  }

  if ( OneWire::crc8(addr, 7) != addr[7]) {
      if (pser) Serial.print(" CRC is not valid!\n");
      return 1;
  } else 
      if (pser) Serial.print(" CRC ok ");
  
  if ( addr[0] != 0x28) {
      if (pser) Serial.print("Device is not a DS18B20 family device.\n");
      return 1;
  } else
      if (pser) Serial.print("DS18B20 ");

  bus1.reset();
  bus1.write(0x55);  // match ROM
  bus1.vwrite(addr,8);
  bus1.write(0x44); // start conversion, with parasite power on at the end
  bus1.power(true);
  
  delay(750);     // maybe 750ms is enough, maybe not
  
  present = bus1.reset();
  bus1.write(0x55);  // match ROM
  bus1.vwrite(addr,8); 
  bus1.write(0xBE);         // Read Scratchpad
  bus1.vread(data, 9);

  if (pser) {
    Serial.print("P=");
    Serial.print(present,HEX);
    Serial.print(" ");
    for ( i = 0; i < 9; i++)
       print_byte(data[i]);
  }
  i = OneWire::crc8(data, 8);
  if (pser) { Serial.print(" CRC=");   print_byte(i); }

  if (i == data[8]) {
      temp = data[0];
      temp += 256.0*data[1];
      temp /= 16.0;
      if (pser) { Serial.print(" temp="); Serial.print(temp); }
      if (plcd) { lcd.print(temp); lcd.print(" ");  }

  }    
  
  if (pser) Serial.println();
  return 1;
}

void analogt(boolean pser, boolean plcd)
{
  int  i,s1,s2;
  float T1,T2;
  //  digitalWrite(led, HIGH);
  for(i=0, s2=s1=0; i<samples; i++) {
    s1 += analogRead(analog_a);
    s2 += analogRead(analog_b);
  }
//  digitalWrite(led, LOW);

// 10mV/K = 0.1 K/mV
  T1 = 0.1 * s1 * refmVolts / (1023.0*samples);
  T2 = 0.1 * s2 * refmVolts / (1023.0*samples);

// T-correction
  T1+= 0.0005*T1-0.261;
  T2+=-0.0027*T2+0.749;

  if (plcd) {
    lcd.print("A" );   lcd.print(T1); 
    lcd.print("B" );   lcd.print(T2);
  }
  if (pser) {
    Serial.print(" Ta:" );   Serial.print(T1);
    Serial.print(" Tb:" );   Serial.print(T2);
//  T2=T2-T1;
//  Serial.print(" Dt:" ); 
//  Serial.print(T2);
  }

}

void ad77xxx(boolean pser, boolean plcd)
{
  float v3;
  unsigned int hh;

    v3 = ad7705.read(AD770X::CHN_AIN1);
//  v3 = ad7705.read(AD770X::CHN_AIN1);
    ad7705.wait(AD770X::CHN_AIN1);
    hh = ad7705.read16(AD770X::REG_DATA|AD770X::CHN_AIN1|AD770X::REG_RD);

  if (plcd) { lcd.print("u"); lcd.print(v3,3); }
  if (pser) { Serial.print(" Ain1:" );  Serial.print(hh,HEX); }
}

void loop()
{

  lcd.setCursor(0, 0);
  analogt(1,1);  
  ad77xxx(1,0);  
  Serial.println("");
  lcd.setCursor(0, 1);
  while(ds18b20(1,1));
  Serial.println("");
  delay(200);
}



