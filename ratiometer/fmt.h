#ifndef __FRMT__
#define __FRMT__

typedef struct _fmt Fmt;

struct _fmt {
	char	*start;			/* of buffer */
	char	*to;			/* current place in the buffer */
	char	*stop;			/* end of the buffer; overwritten if flush fails */
	int	(*flush)(Fmt *);	/* called when to == stop */
	void	*farg;			/* to make flush a closure */
	int	nfmt;			/* num chars formatted so far */
	va_list	args;			/* args passed to dofmt */
	int	r;			/* % format Rune */
	int	width;
	int	prec;
	int	flags;
};

enum {
	FmtWidth	= 1,
	FmtLeft		= 2,
	FmtPrec		= 4,
	FmtSharp	= 8,
	FmtSpace	= 16,
	FmtSign		= 32,
	FmtZero		= 64,
	FmtUnsigned	= 128,
	FmtShort	= 256,
	FmtLong		= 512,
	FmtVLong	= 1024,
	FmtComma	= 2048,
	FmtByte		= 4096,
	FmtFlag		= 8192,
};

int cprintf(char *fmt, ...);
int sprintf(char *buf, char *fmt, ...);

#endif
