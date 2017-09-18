#include <Arduino.h>
#include <stdarg.h>

void print_hex2(uint8_t x)
{
  if (x < 0x10) Serial.print('0');
  Serial.print(x,HEX);
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
      case 'x':  
            int m;
            m = va_arg(argv, int);
            if (prec<=1) Serial.print(m,HEX);
            if (prec==2) print_hex2(m);
            if (prec==4){ print_hex2(highByte(m));
                          print_hex2(lowByte(m));  }
            break;
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

