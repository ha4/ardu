#ifndef XXSTPP
#define XXSTPP
enum {   MOTOR_STEPS=200, MOTOR_uSTEPS=16, MOTOR_Ipk=1400 };
#endif

extern int STP_number, STP_speed, STP_dir, STP_mA, STP_Gsens, STP_reverse;
extern int STP_npole, STP_usteps;
extern int number_of_steps, STP_step;
extern int running, run_amount;

void Stepper_init();
void setSpeed(int rph, int steps, int ustep);
void setCurrent(int mA, int sm); // milliAmpere, simens
void stepMotor(int thisStep); // 0..63
void rawStepper(int dir);
int  pulseStepper();
void stepStepper(int steps_to_move);

