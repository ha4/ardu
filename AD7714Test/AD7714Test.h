/*
   Mean  M = sum(x[i])/N = Sx/N
 StdDev SN = sqrt(sum(Dx[i]^2)/N)=sqrt(sum((x[i]-M)^2)/N)=sqrt(sum( x[i]^2 - 2*M*x[i] + M^2 )/N)=
 sqrt(sum(x[i]^2)/N - 2*M^2 + M^2)=sqrt(sum(x[i]^2)/N - M^2)=1/N*sqrt(N*Sxx - Sx*Sx)
*/


class average {
  public:
  average() {  clear(); };
  void clear() { N = 0; sx=0; sxx=0; };
  void sample(double x) { N++; sx+=x; x-=sx/N; sxx+=x*x; };
  double avg() { return sx/N; };
  double stddev() { return sqrt(sxx/N); };
  long N;
  double sx;
  double sxx;
};


