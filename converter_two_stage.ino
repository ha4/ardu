//Sets the frequency for timer 1
#define TIMER_TOP_1 100 // f=(F_CPU/TIMER_TOP_1)

// Limits the maximum output voltage
#define MAX_BUCK_DUTY_0 1 // Converter becomes buck mode if MAX_BUCK_DUTY_n is below 1
#define MAX_BUCK_DUTY_1 1
#define MAX_BUCK_DUTY_2 1
#define MAX_BOOST_DUTY_0 0.6
#define MAX_BOOST_DUTY_1 0.6
#define MAX_BOOST_DUTY_2 0.6

// Soft starts the converter
const int minOutput = 0; 

// Resolutions for timers 0 and 2. Must be set to 255
#define TIMER_RESOLUTION_0 255 
#define TIMER_RESOLUTION_2 255 

// Calculates the maximum duty cycles
const int maxBuck0 = TIMER_RESOLUTION_0 * MAX_BUCK_DUTY_0;
const int maxBuck1 = TIMER_TOP_1 * MAX_BUCK_DUTY_1;
const int maxBuck2 = TIMER_RESOLUTION_2 * MAX_BUCK_DUTY_2;
const int maxBoost0 = TIMER_RESOLUTION_0 * MAX_BOOST_DUTY_0;
const int maxBoost1 = TIMER_TOP_1 * MAX_BOOST_DUTY_1;
const int maxBoost2 = TIMER_RESOLUTION_2 * MAX_BOOST_DUTY_2;

//Cut-off duty cycles for using buck and boost modes
#define CUT_OFF_BUCK_DUTY_HIGH 1
#define CUT_OFF_BOOST_DUTY_LOW 0

const int desiredFeedbackVoltage=500; //Sets the feedback voltage in mV
const float percentVoltage=desiredFeedbackVoltage/1100.0;
const int desiredFeedback=percentVoltage*1023;

const int boostLowCutoff=0;
const int buckHighCutoff0 = TIMER_RESOLUTION_0 * CUT_OFF_BUCK_DUTY_HIGH;
const int buckHighCutoff1 = TIMER_TOP_1 * CUT_OFF_BUCK_DUTY_HIGH;
const int buckHighCutoff2 = TIMER_RESOLUTION_2 * CUT_OFF_BUCK_DUTY_HIGH;

int output0a, output0b, output1a, output1b, output2a, output2b;
int feedback0,feedback1,feedback2;

// Start with buck mode
boolean stepUpMode0 = false;
boolean stepUpMode1 = false;
boolean stepUpMode2 = false;

byte reEnableOutput;

#define FEEDBACK_PIN_0 0
#define FEEDBACK_PIN_1 1
#define FEEDBACK_PIN_2 2

#define BUCK_0_PWM_PIN 6
#define BOOST_0_PWM_PIN 5
#define BUCK_1_PWM_PIN 9
#define BOOST_1_PWM_PIN 10
#define BUCK_2_PWM_PIN 11
#define BOOST_2_PWM_PIN 3


void setup(){
// Set timers 0, 1, and 2 to non-inverting fast PWM mode with prescalers of 1
TCCR0A = _BV(COM0A1)|_BV(COM0B1)|_BV(WGM01)|_BV(WGM00);
TCCR0B = _BV(CS00);
TCCR1A = _BV(COM1A1)|_BV(COM1B1)|_BV(WGM11);
TCCR1B = _BV(WGM12)|_BV(WGM13)|_BV(CS10);
TCCR2A = _BV(COM2A1)|_BV(COM2B1)|_BV(WGM21)|_BV(WGM20);
TCCR2B = _BV(CS20);

analogReference(INTERNAL);// Sets the analog reference to 1.1V

// Turn on the ADC pins for channels 0, 1 and 2
pinMode(FEEDBACK_PIN_0,INPUT);
pinMode(FEEDBACK_PIN_1,INPUT);
pinMode(FEEDBACK_PIN_2,INPUT);

// Turn on the buck and boost PWM pins for channels 0, 1 and 2. Uncomment to reenable.
pinMode(BUCK_0_PWM_PIN,OUTPUT);
pinMode(BOOST_0_PWM_PIN,OUTPUT);
//pinMode(BUCK_1_PWM_PIN,OUTPUT);
//pinMode(BOOST_1_PWM_PIN,OUTPUT);
//pinMode(BUCK_2_PWM_PIN,OUTPUT);
//pinMode(BOOST_2_PWM_PIN,OUTPUT);


// Start at the lowest duty cycle
OCR0A = minOutput;
OCR0B = minOutput;
OCR1A = minOutput;
OCR1B = minOutput;
OCR2A = minOutput;
OCR2B = minOutput;

ICR1 =TIMER_TOP_1;

// Start with buck mode
digitalWrite(BOOST_0_PWM_PIN,LOW);
digitalWrite(BOOST_1_PWM_PIN,LOW);
digitalWrite(BOOST_2_PWM_PIN,LOW);
}

void loop(){

  feedback0 = analogRead(FEEDBACK_PIN_0);
  feedback1 = analogRead(FEEDBACK_PIN_1);
  feedback2 = analogRead(FEEDBACK_PIN_2);
  
  if (stepUpMode0==false){ // if mode is buck
    output0a = setOutput(feedback0,maxBuck0,output0a);
    OCR0A=output0a;
    
    if(output0a >= buckHighCutoff0 && feedback0 < desiredFeedback){
      reEnableOutput=setRegister(stepUpMode0,BUCK_0_PWM_PIN);
      TCCR0A= TCCR0A | reEnableOutput;
      stepUpMode0 = !stepUpMode0; // Switch to boost mode    
     }
  }
  else if(stepUpMode0==true){ // If mode is boost
    output0b = setOutput(feedback0,maxBoost0,output0b);
    OCR0B=output0b;
    
    if (output0b <= boostLowCutoff && feedback0 > desiredFeedback){ 
      reEnableOutput=setRegister(stepUpMode0,BOOST_0_PWM_PIN);
      TCCR0A = TCCR0A | reEnableOutput;
      stepUpMode0 = !stepUpMode0; // Switch to buck mode
     }
  }
  
  if (stepUpMode1==false){
    output1a = setOutput(feedback1,maxBuck1,output1a);
    OCR1A=output1a;
    
    if(output1a >= buckHighCutoff1 && feedback1 < desiredFeedback){
      reEnableOutput=setRegister(stepUpMode1,BUCK_1_PWM_PIN);
      TCCR1A= TCCR1A | reEnableOutput;
      stepUpMode1 = !stepUpMode1;
     }
  }
  else if(stepUpMode1==true){
    output1b = setOutput(feedback1,maxBoost1,output1b);
    OCR1B=output1b;
    
    if (output1b <= boostLowCutoff && feedback1 > desiredFeedback){ 
      reEnableOutput=setRegister(stepUpMode1,BOOST_1_PWM_PIN);
      TCCR1A = TCCR1A | reEnableOutput;
      stepUpMode1 = !stepUpMode1;
     }
  }

  if (stepUpMode2==false){
    output2a = setOutput(feedback2,maxBuck2,output2a);
    OCR2A=output2a;
    
    if(output2a >= buckHighCutoff2 && feedback2 < desiredFeedback){
      reEnableOutput=setRegister(stepUpMode2,BUCK_2_PWM_PIN);
      TCCR2A= TCCR2A | reEnableOutput;
      stepUpMode2 = !stepUpMode2;
     }
  }
  else if(stepUpMode2==true){
    output2b = setOutput(feedback2,maxBoost2,output2b);
    OCR2B=output2b;
    
    if (output2b <= boostLowCutoff && feedback2 > desiredFeedback){ 
      reEnableOutput=setRegister(stepUpMode2,BOOST_2_PWM_PIN);
      TCCR2A = TCCR2A | reEnableOutput;
      stepUpMode2 = !stepUpMode2;
     }
  }  
}

int setOutput(int feedback, int maxOutput, int output){
  if(feedback>desiredFeedback && output > minOutput){
   output--;
  }
  
  else if(feedback<desiredFeedback && output < maxOutput ){
   output++;
  }

  return output;
}

int setRegister(boolean mode, int pin){
  if (mode ==false){
    digitalWrite(pin,HIGH); //Disables the buck converter
    reEnableOutput= B00100000; //Re-enables boost converter
  }
  
  else if (mode ==true){
    digitalWrite(pin,LOW); // Disables the boost converter
    reEnableOutput= B10000000; //Re-enables the buck converter
  }
  
  return reEnableOutput;
}
