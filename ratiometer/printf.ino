#include <Arduino.h>
#include <stdarg.h>
#include "fmt.h"

int _ifmt(Fmt *f);
int _efgfmt(Fmt *f);

#define FMTCHAR(f, t, s, c)\
	do{\
	if(t + 1 > s){\
		t = _fmtflush(f, t, 1);\
		if(t != 0) s = f->stop; else return -1;\
	}\
	*t++ = c;\
	}while(0)

char* _fmtflush(Fmt *f, char *t, int len)
{
	f->nfmt += t - f->to;
	f->to = t;
	if(f->flush == 0 || (*f->flush)(f) == 0 || f->to + len > f->stop){
		f->stop = f->to;
		return 0;
	}
	return f->to;
}

/*
 * put a formatted block of memory sz bytes long of n runes into the output buffer,
 * left/right justified in a field of at least f->width charactes
 */
int
_fmtpad(Fmt *f, int n)
{
	char *t, *s;
	int i;

	t = f->to;
	s = f->stop;
	for(i = 0; i < n; i++)
		FMTCHAR(f, t, s, ' ');
	f->nfmt += t - (char *)f->to;
	f->to = t;
	return 0;
}

/* returns: 0:finished -1:error */
int _fmtcpy(Fmt *f, char *vm, int n, int sz)
{
	char *t, *s, *m, *me;
	int fl;
	int nc, w, r;

	m = vm;
	me = m + sz;
	w = f->width;
	fl = f->flags;
	if((fl & FmtPrec) && n > f->prec)
		n = f->prec;

	if(!(fl & FmtLeft) && _fmtpad(f, w - n) < 0)
		return -1;
	t = f->to;
	s = f->stop;
	for(nc = n; nc > 0; nc--){
		r = *(unsigned char*)m++;
		FMTCHAR(f, t, s, r);
	}
	f->nfmt += t - (char *)f->to;
	f->to = t;
	if(fl & FmtLeft && _fmtpad(f, w - n) < 0)
		return -1;
	return 0;
}

/* returns 1:continue 0:format finished -1:format error */
static int fmtfmt(Fmt *f)
{
  char x[3];
  switch(f->r) {
	case ',': f->flags |= FmtComma;	return 1; /* flag fmt group */
	case '-': f->flags |= FmtLeft;	return 1;
	case '+': f->flags |= FmtSign;	return 1;
	case '#': f->flags |= FmtSharp;	return 1;
	case ' ': f->flags |= FmtSpace;	return 1;
	case 'u': f->flags |= FmtUnsigned;return 1;
	case 'h': if(f->flags & FmtShort) f->flags |= FmtByte; f->flags |= FmtShort; return 1;
	case 'l': if(f->flags & FmtLong) f->flags |= FmtVLong; f->flags |= FmtLong; return 1;

	case '%': x[0] = f->r; f->prec = 1; return _fmtcpy(f, x, 1, 1); /* fmt a % */
	case 'E': case 'G': case 'e': case 'f': case 'g': return _efgfmt(f);
	case 'X': case 'b': case 'd': case 'o': case 'p': case 'x': return _ifmt(f);
	case 'c': x[0] = va_arg(f->args, int); f->prec = 1; return _fmtcpy(f, x, 1, 1); /* fmt out one character */
	case 's': /* fmt out a null terminated string */
		char *s; int i, j;
		s = va_arg(f->args, char *);
		if(!s) return _fmtcpy(f, "<nil>", 5, 5);
		/* if precision is specified, make sure we don't wander off the end */
		if(f->flags & FmtPrec){ for(i=j=0; j < f->prec && s[i]; j++) i++; }
		else  for(i=j=0; s[i]; j++) i++;
		return _fmtcpy(f, s, j, i);
  }
	/* default error format */
	x[0] = '%'; x[1] = f->r; x[2] = '%';
	f->prec = 3;
	_fmtcpy(f, x, 3, 3);
	return 0;
}

char* _fmtdispatch(Fmt *f, char *fmt)
{
	int r;
	int i, n;

	f->flags = 0;
	f->width = f->prec = 0;

	for(;;){
		r = *(unsigned char*) fmt++;
		f->r = r;
		switch(r){
		case '\0': return 0;
		case '.': f->flags |= FmtWidth|FmtPrec;		continue;
		case '0': if(!(f->flags & FmtWidth)) { f->flags |= FmtZero; continue; }
			/* fall through */
		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			i = 0;
			while(r >= '0' && r <= '9'){
				i = i * 10 + r - '0';
				r = (unsigned char)*fmt++;
			}
			fmt--;
		numflag:
			if(f->flags & FmtWidth){ f->flags |= FmtPrec; f->prec = i; }
			else { f->flags |= FmtWidth; f->width = i; }
			continue;
		case '*':
			i = va_arg(f->args, int);
			if(i < 0) { /* negative precision => ignore the precision.  */
				if(f->flags & FmtPrec){	f->flags &= ~FmtPrec; f->prec = 0; continue; }
				i = -i;
				f->flags |= FmtLeft;
			}
			goto numflag;
		}
		n = fmtfmt(f);
		if(n < 0)
			return 0;
		if(n == 0)
			return fmt;
	}
}

int dofmt(Fmt *f, char *fmt)
{
	int r;
	char *t, *s;
	int n, nfmt;

	nfmt = f->nfmt;
	for(;;){
		t = f->to;
		s = f->stop;
		while((r = *(unsigned char*)fmt) && r != '%'){
  			FMTCHAR(f, t, s, r);
			fmt++;
		}
		fmt++;
		f->nfmt += t - f->to;
		f->to = t;
		if(!r)
			return f->nfmt - nfmt;
		f->stop = s;

		fmt = _fmtdispatch(f, fmt);
		if(fmt == 0)
			return -1;
	}
}


/* 
 * interface implementation
 */

int _fmtSerialFlush(Fmt *f)
{
	int n;

	n = (char*)f->to - (char*)f->start;
	if(n) Serial.write((uint8_t*)f->start, n);
	f->to = f->start;
	return 1;
}

int cprintf(char *fmt, ...)
{
	Fmt f;
	char buf[20];
	int n;
	va_list args;

	va_start(args, fmt);

	f.start = buf;
	f.to = buf;
	f.stop = buf + sizeof(buf);
	f.flush = _fmtSerialFlush;
	f.nfmt = 0;

	f.args = args;

	n = dofmt(&f, fmt);
	va_end(args);

	if(n > 0 && (*f.flush)(&f) == 0)
		return -1;
	return n;
}


int sprintf(char *buf, char *fmt, ...)
{
 	Fmt f;
 	va_list args;

	va_start(args, fmt);
	f.start = buf;
	f.to = buf;
	f.stop = buf + 2047;
	f.flush = 0;
	f.nfmt = 0;
	f.args = args;
	dofmt(&f, fmt);
	va_end(args);

	*f.to = '\0';
	return f.to - buf;
}

/* 
 * format implementation
 */

/* fmt an integer */
#define INTCONV(v)\
	while(v) { \
	i = v % base; v /= base; \
	if((fl & FmtComma) && n % 4 == 3){ *p-- = ','; n++; } \
	*p-- = conv[i]; n++; \
}

#define noLONGLONGCVT

int _ifmt(Fmt *f)
{
	char buf[16], *p, *conv;
#ifdef LONGLONGCVT
	unsigned long long vu=0;
#endif
	unsigned long u=0;
	unsigned int *pu;
	int neg = 0, base, i, n, fl = f->flags, w, isv = 0;

	if(f->r == 'p'){
		pu = va_arg(f->args, unsigned int *);
#ifdef LONGLONGCVT
		if(sizeof(unsigned int *) == sizeof(unsigned long long)){
			vu = (unsigned long long)pu;
			isv = 1;
		}else
#endif
			u = (unsigned long)pu;
		f->r = 'x';
		fl |= FmtUnsigned;
#ifdef LONGLONGCVT
	}else if(fl & FmtVLong){
		isv = 1;
		if(fl & FmtUnsigned)
			vu = va_arg(f->args, unsigned long long);
		else
			vu = va_arg(f->args, long long);
#endif
	}else if(fl & FmtLong){
		if(fl & FmtUnsigned)
			u = va_arg(f->args, unsigned long);
		else
			u = va_arg(f->args, long);
	}else if(fl & FmtByte){
		if(fl & FmtUnsigned)
			u = (unsigned char)va_arg(f->args, int);
		else
			u = (char)va_arg(f->args, int);
	}else if(fl & FmtShort){
		if(fl & FmtUnsigned)
			u = (unsigned short)va_arg(f->args, int);
		else
			u = (short)va_arg(f->args, int);
	}else{
		if(fl & FmtUnsigned)
			u = va_arg(f->args, unsigned int);
		else
			u = va_arg(f->args, int);
	}

	conv = "0123456789abcdef";
	switch(f->r){
	case 'd': base = 10; break;
	case 'X': conv = "0123456789ABCDEF";
	case 'x': base = 16; break;
	case 'b': base = 2; break;
	case 'o': base = 8; break;
	default: return -1;
	}

	if(!(fl & FmtUnsigned)){
#ifdef LONGLONGCVT
		if(isv && (long long)vu < 0) { vu = -(long long)vu; neg = 1; } else 
#endif
		if(!isv && (long)u < 0){ u = -(long)u; neg = 1; }
	}
	p = buf + sizeof buf - 1;
	n = 0;
#ifdef LONGLONGCVT
	if(isv){
		INTCONV(vu);
	}else
#endif
		INTCONV(u);

	if(n == 0){ *p-- = '0'; n = 1; }
	for(w = f->prec; n < w && p > buf+3; n++) *p-- = '0';
	if(neg || (fl & (FmtSign|FmtSpace))) n++;
	if(fl & FmtSharp){
		if(base == 16)
			n += 2;
		else if(base == 8){
			if(p[1] == '0')
				fl &= ~FmtSharp;
			else
				n++;
		}
	}
	if((fl & FmtZero) && !(fl & (FmtLeft|FmtPrec))){
		for(w = f->width; n < w && p > buf+3; n++)
			*p-- = '0';
		f->width = 0;
	}
	if(fl & FmtSharp){
		if(base == 16)
			*p-- = f->r;
		if(base == 16 || base == 8)
			*p-- = '0';
	}
	if(neg)
		*p-- = '-';
	else if(fl & FmtSign)
		*p-- = '+';
	else if(fl & FmtSpace)
		*p-- = ' ';
	f->flags &= ~FmtPrec;
	return _fmtcpy(f, p + 1, n, n);
}

/* fmt a float */
int _efgfmt(Fmt *f)
{
	double d;
	char s[30];
        int j;

	d = va_arg(f->args, double);

	// xdtoa(fmt, s, f);
	dtostrf(d, f->width, f->prec, s);
	f->flags &= FmtWidth|FmtLeft;
	for(j=0; s[j]; j++);
	_fmtcpy(f, s, j, j);
	return 0;
}

