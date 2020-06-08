//Used to set timer 1's PWM frequency
#define TIMER_TOP_1 100 // f=(F_CPU/TIMER_TOP_1)

// Sets the maximum duty cycle
#define MAX_DUTY_0A 0.6
#define MAX_DUTY_0B 0.6
#define MAX_DUTY_1A 0.6
#define MAX_DUTY_1B 0.6
#define MAX_DUTY_2A 0.6
#define MAX_DUTY_2B 0.6

//Sets the reference voltage in mV
const int desiredFeedbackVoltage = 500;
const float percentVoltage = desiredFeedbackVoltage/1100.0;
const int desiredFeedback = percentVoltage*1023;

// Soft starts the converter
const int minOutput = 0; 

// Resolutions for timers 0 and 2. Must be set to 255
#define TIMER_RESOLUTION_0 255 
#define TIMER_RESOLUTION_2 255

//Calculates the maximum PWM values
const int maxOutput0a = TIMER_RESOLUTION_0 * MAX_DUTY_0A; 
const int maxOutput0b = TIMER_RESOLUTION_0 * MAX_DUTY_0B; 
const int maxOutput1a = TIMER_TOP_1 * MAX_DUTY_1A;
const int maxOutput1b = TIMER_TOP_1 * MAX_DUTY_1B;
const int maxOutput2a = TIMER_RESOLUTION_2 * MAX_DUTY_2A; 
const int maxOutput2b = TIMER_RESOLUTION_2 * MAX_DUTY_2B; 

int output0a,output0b,output1a,output1b,output2a,output2b;
int feedback0a,feedback0b,feedback1a,feedback1b,feedback2a,feedback2b;

#define FEEDBACK_PIN_0A 0
#define FEEDBACK_PIN_0B 1
#define FEEDBACK_PIN_1A 2
#define FEEDBACK_PIN_1B 3
#define FEEDBACK_PIN_2A 4
#define FEEDBACK_PIN_2B 5

void setup(){
// Set to non-inverting fast PWM mode with a prescaler of 1
TCCR0A = _BV(COM0A1)|_BV(COM0B1)|_BV(WGM01)|_BV(WGM00);
TCCR0B = _BV(CS00);
TCCR1A = _BV(COM1A1)|_BV(COM1B1)|_BV(WGM11);
TCCR1B = _BV(WGM12)|_BV(WGM13)|_BV(CS10);
TCCR2A = _BV(COM2A1)|_BV(COM2B1)|_BV(WGM21)|_BV(WGM20);
TCCR2B = _BV(CS20);

analogReference(INTERNAL);// Sets the analog reference to 1.1V

// Set the ADC pins as inputs
pinMode(FEEDBACK_PIN_0A,INPUT); 
pinMode(FEEDBACK_PIN_0B,INPUT);
pinMode(FEEDBACK_PIN_1A,INPUT);
pinMode(FEEDBACK_PIN_1B,INPUT);
pinMode(FEEDBACK_PIN_2A,INPUT);
pinMode(FEEDBACK_PIN_2B,INPUT);

// Turn on the PWM pins OC0A, 0C1A OC1A, OC1B, OC2A, OC2B. Uncomment to reenable.
pinMode(6,OUTPUT); 
//pinMode(5,OUTPUT);
//pinMode(9,OUTPUT);
//pinMode(10,OUTPUT);
//pinMode(11,OUTPUT);
//pinMode(3,OUTPUT);

// Start at the lowest duty cycle
OCR0A = minOutput; 
OCR0B = minOutput;
OCR1A = minOutput;
OCR1B = minOutput;
OCR2A = minOutput;
OCR2B = minOutput;

ICR1 =TIMER_TOP_1;
}

void loop(){
 
  // Read the feedback pins
  feedback0a = analogRead(FEEDBACK_PIN_0A);
  feedback0b = analogRead(FEEDBACK_PIN_0B);
  feedback1a = analogRead(FEEDBACK_PIN_1A);
  feedback1b = analogRead(FEEDBACK_PIN_1B);
  feedback2a = analogRead(FEEDBACK_PIN_2A);
  feedback2b = analogRead(FEEDBACK_PIN_2B);
  
 output0a= setOutput(output0a,feedback0a,maxOutput0a);
 output0b= setOutput(output0b,feedback0b,maxOutput0b);
 output1a= setOutput(output1a,feedback1a,maxOutput1a);
 output1b= setOutput(output1b,feedback1b,maxOutput1b);
 output2a= setOutput(output2a,feedback2a,maxOutput2a);
 output2b= setOutput(output2b,feedback2b,maxOutput2b);
 
 // Update the duty cycles
 OCR0A =output0a;
 OCR0B =output0b;
 OCR1A =output1a;
 OCR1B =output1b;
 OCR2A =output2a;
 OCR2B =output2b;
}

// Increase or decrease duty cycle based on feedback voltage and PWM level
int setOutput(int currentOutput,int feedback,int maxOutput){
  if(feedback > desiredFeedback && currentOutput >minOutput){
    currentOutput--;
  }
  
  else if(feedback < desiredFeedback && currentOutput <maxOutput){
    currentOutput++;
  }
  
  return currentOutput;
}
