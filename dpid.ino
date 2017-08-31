
#define PID_MAX 1024

int pid_sp;
int pid_pb;
int pid_si;
int pid_di;
int32_t pid_i;
int32_t pid_e;

int pid(int pv)
{
	int32_t y;
	int32_t e;
	int32_t imax;

	e = pv - pid_sp;
	pid_e = e-pid_e;

	pid_i += e * (PID_MAX-1) / pid_pb;
	imax = PID_MAX*pid_si - 1;
	if (pid_i > imax) pid_i=imax; else if (pid_i < -imax) pid_i=-imax;

	y = e * PID_MAX / pb;
	y += pid_i  / si;
	y += pid_e * PID_MAX * pid_di / pb;
	pid_e = e;
	if (y > (PID_MAX-1)) y=PID_MAX-1; else if (y < 0) y=0;

	return y;
}