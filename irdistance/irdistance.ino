enum { pinn=4, pinf=3, pint=A0, ping=16};
int cprintf(char *str, ...);

const uint8_t navg=32;
class ravg {
  public:
  uint8_t index;
  uint32_t sum;
  uint16_t values[navg];
  ravg(): index(0), sum(0) { }
  void add(uint16_t v) { sum = sum + v - values[index]; values[index++]=v; if (index >= navg) index = 0; }
  void init() { for(uint8_t i=0; i < navg; i++) values[i]=0; index=0; sum=0; }
} n,f,z;
uint8_t s;

void setup()
{
  digitalWrite(pinn, 1);
  pinMode(pinn, OUTPUT);
  digitalWrite(pinf, 1);
  pinMode(pinf, OUTPUT);
  pinMode(ping, OUTPUT);
  digitalWrite(ping, 0);
  analogReference(INTERNAL);
  Serial.begin(115200);
  s=0;
  z.init();
  n.init();
  f.init();
}

uint8_t every100ms() {
  static uint32_t t1=0;
  uint32_t tn=millis();
  if (tn-t1 >= 100) { t1=tn; return 1; }
  return 0;
}

void loop()
{
  switch(s++) {
    case 1:
      analogRead(pint);
      analogRead(pint);
      z.add(analogRead(pint));
      digitalWrite(pinn, 0);
      break;
    case 2:
      analogRead(pint);
      analogRead(pint);
      n.add(analogRead(pint));
      digitalWrite(pinn, 1);
      digitalWrite(pinf, 0);
      break;

    default:
      s=0;
    case 0:
      analogRead(pint);
      analogRead(pint);
      f.add(analogRead(pint));
      digitalWrite(pinf, 1);
  }
  if (every100ms()) cprintf("%ld %ld %ld\n", z.sum, n.sum-z.sum, f.sum-z.sum);
}


int cprintf(char *str, ...)
{
  int prec;
  va_list argv;

  va_start(argv, str);
  for(; *str; str++) {
    if(*str=='%') {
      prec = -1;
  chksym:
      switch(*++str) {
      case 'd':  Serial.print(va_arg(argv, int));   break;
      case 'x':  Serial.print(va_arg(argv, int),HEX);  break;
      case 'l':  Serial.print(va_arg(argv, long));    break;
      case 'f':  if(prec<0)prec=2; Serial.print(va_arg(argv, double),prec);  break;
      case 'c':  Serial.print((char)va_arg(argv, int));   break;
      case 's':  Serial.print(va_arg(argv, char *));   break;
      default: if (*str >='0' && *str <='9') { prec=*str-'0'; goto chksym; }
               goto emitsym;
      }
    } else {
        emitsym:
        Serial.print(*str);
    }
  }
  return 1;
}


