#include <Arduino.h>

/* return r^2, data: a,b, x1,y1, x2,y2... */
float fit(float *data, int n)
{
	double syy, sxx, sxy, sx, sy, *a, *b;
	if (n < 1) return 0;
	syy=sxx=sxy=sx=sy=0;
	a = data++;
	b = data++;
	for (int i = 1; i<=n; i++) {
		double x=*data++;
		double y=*data++;
		sx+=x;
		sy+=y;
		sxx+=x*x;
		sxx+=y*y;
		sxy+=x*y;
	}
	double pxx = sxx - sx*sx/n;
	double pxy = sxy - sx*sy/n;
	double pyy = syy - sy*sy/n;
	*a = pxy/pxx;
	*b = sy/n - *a * sx/n;
	return pxy*pxy/(pxx*pyy);
}
