
void timer_mode8(uint8_t wgm)
{
  switch(wgm) {
  default: Serial.println(" Unknown"); break;
  case 0: Serial.println(" Normal, TOP=FF"); break;
  case 1: Serial.println(" PWM, PH.CORR, TOP=FF"); break;
  case 2: Serial.println(" CTC, TOP=OCR-A"); break;
  case 3: Serial.println(" PWM, Fast, TOP=FF"); break;
  case 4: Serial.println(" reserv"); break;
  case 5: Serial.println(" PWM, PH.CORR, TOP=OCR-A"); break;
  case 6: Serial.println(" reserv"); break;
  case 7: Serial.println(" PWM, Fast, TOP=OCR-A"); break;
  }
}
void timer_mode16(uint8_t wgm)
{
  switch(wgm) {
  default: Serial.println(" Unknown"); break;
  case 0: Serial.println(" Normal, TOP=FFFF"); break;
  case 1: Serial.println(" PWM, PH.CORR, TOP=00FF"); break;
  case 2: Serial.println(" PWM, PH.CORR, TOP=01FF"); break;
  case 3: Serial.println(" PWM, PH.CORR, TOP=03FF"); break;
  case 4: Serial.println(" CTC, TOP=OCR-A"); break;
  case 5: Serial.println(" PWM, Fast, TOP=00FF"); break;
  case 6: Serial.println(" PWM, Fast, TOP=01FF"); break;
  case 7: Serial.println(" PWM, Fast, TOP=03FF"); break;
  case 8: Serial.println(" PWM, PH.FREQ.CORR, TOP=ICR"); break;
  case 9: Serial.println(" PWM, PH.FREQ.CORR, TOP=OCR-A"); break;
  case 10: Serial.println(" PWM, PH.CORR, TOP=ICR"); break;
  case 11: Serial.println(" PWM, PH.CORR, TOP=OCR-A"); break;
  case 12: Serial.println(" CTC, TOP=ICR"); break;
  case 13: Serial.println(" reserv"); break;
  case 14: Serial.println(" PWM, Fast, TOP=ICR"); break;
  case 15: Serial.println(" PWM, Fast, TOP=OCR-A"); break;
  }
}

void com_mode(uint8_t com)
{
  switch(com){
  case 0: Serial.println(" Disable"); break;
  case 1: Serial.println(" Toggle at OC match"); break;
  case 2: Serial.println(" Clear at OC match"); break;
  case 3: Serial.println(" Set at OC match"); break;
  }
}

void cs_mode1(uint8_t cs)
{
  switch(cs){
  case 0: Serial.println(" Disable"); break;
  case 1: Serial.println(" CLK"); break;
  case 2: Serial.println(" CLK/8"); break;
  case 3: Serial.println(" CLK/64"); break;
  case 4: Serial.println(" CLK/256"); break;
  case 5: Serial.println(" CLK/1024"); break;
  case 6: Serial.println(" Tpin-falling"); break;
  case 7: Serial.println(" Tpin-rising"); break;
  }
}

void cs_mode2(uint8_t cs)
{
  switch(cs){
  case 0: Serial.println(" Disable"); break;
  case 1: Serial.println(" CLK"); break;
  case 2: Serial.println(" CLK/8"); break;
  case 3: Serial.println(" CLK/32"); break;
  case 4: Serial.println(" CLK/64"); break;
  case 5: Serial.println(" CLK/128"); break;
  case 6: Serial.println(" CLK/256"); break;
  case 7: Serial.println(" CLK/1024"); break;
  }
}

void timer_decode(uint8_t ra, uint8_t rb, uint8_t rt, uint8_t num)
{
    uint8_t coma = ra>>6;
    uint8_t comb = (ra>>4)&3;
    uint8_t wgm  = (ra&3)|((rb>>1)&12);
    uint8_t cs   = rb&7;
    uint8_t icnc = rb>>7;
    uint8_t ices = (rb>>6)&1;
    uint8_t icie = (rt>>5)&1;
    uint8_t ocieb= (rt>>2)&1;
    uint8_t ociea= (rt>>1)&1;
    uint8_t toie = rt&1;
    Serial.print("--TIMER"); Serial.println(num);
    Serial.print("COMa:"); Serial.print(coma,BIN); com_mode(coma);
    Serial.print("COMb:"); Serial.print(comb,BIN); com_mode(comb);
    Serial.print("WGM:"); Serial.print(wgm,BIN); if (num==1)timer_mode16(wgm); else timer_mode8(wgm);
    Serial.print("CS:"); Serial.print(cs,BIN); if (num==2)cs_mode2(cs); else cs_mode1(cs);
    Serial.print("ICNC/FOC0A:"); Serial.println(icnc,BIN);
    Serial.print("ICES/FOC0B:"); Serial.println(ices,BIN);
    Serial.print("OCIEB:"); Serial.println(ocieb,BIN);
    Serial.print("OCIEA:"); Serial.println(ociea,BIN);
    Serial.print("TOIE:"); Serial.println(toie,BIN);
}

void setup()
{
  Serial.begin(57600);
  TCCR1B = (TCCR1B&~7)|1;
}

uint32_t tt;
void loop()
{
  uint32_t t=millis();
  if (t-tt > 750) {
    tt=t;
    timer_decode(TCCR0A, TCCR0B, TIMSK0,0);
    timer_decode(TCCR1A, TCCR1B, TIMSK1,1);
    timer_decode(TCCR2A, TCCR2B, TIMSK2,2);
  }
}

