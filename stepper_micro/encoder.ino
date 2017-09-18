enum { 
  enc_a=16, enc_0, enc_b, enc_button,
};

int enc_prev;
long enc_timr;
int bttn_st;
long bttn_timr;
long bttn_timr2;

void enc_init()
{
  pinMode(enc_0, OUTPUT);
  digitalWrite(enc_0, 0);
  pinMode(enc_a, INPUT_PULLUP);
  pinMode(enc_b, INPUT_PULLUP);
  pinMode(enc_button, INPUT_PULLUP);
  enc_timr=millis();
  bttn_timr=millis();
  bttn_timr2=millis();
  enc_prev=1;
  bttn_st=0;
}

int  inputRot()
{
  volatile int a;

  if (millis() - enc_timr < ENC_DENOISE) return ENC_NULL;

  enc_timr = millis();

  a = digitalRead(enc_a);
  a ^= enc_prev;  // a=in xor prev
  enc_prev ^=a;   // prev = prev xor a = prev xor in xor prev = in
  if (a==1 && enc_prev==0) { // change & in_is_0
    if (digitalRead(enc_b)==0)
      return ENC_CCW;
    else
      return ENC_CW;
  }
  return ENC_NULL;
}

int inputBttn()
{
  int level = digitalRead(enc_button);
  long now = millis();

  if (now - bttn_timr2 > ENC_TDENOISE) bttn_timr2=now;
  else return ENC_NULL;
  
  switch (bttn_st)
  {
  case 0: 
    if (level==0) {
      bttn_st=1; 
      bttn_timr = now;
    } 
    break;
  case 1: 
    if (level==1)
      bttn_st=2; 
    else if (now - bttn_timr > ENC_TLONG) { 
      bttn_st=6; 
      return ENC_LONG;
    }
    break;
  case 2: 
    if(now - bttn_timr > ENC_TCLICK) {
      bttn_st=0;
      return ENC_CLICK;
    }
    if (level == 0) bttn_st=3;
    break;
  case 3:
    if (level == 1) {
      bttn_st = 0;
      return ENC_DOUBLE;
    }
  case 6:
    if(level == 1) {
      bttn_st = 0;
      return ENC_RELEASE;
    }
  }
  return ENC_NULL;
}





