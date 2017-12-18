
/*
  plulse ---------------------.
            ,--------[15k]----|--------------------.
            `-#1        #4----'                    |
            .-#2 PC814  #3--GND                    |
            `----------[15k]-----------.           |
                                       |           |
  out-[220]--#1         #6----------T1-+-----------|---o L
  gnd--------#2 MOC3052 #5          T2------HEATER-+---o N
             #3         #4 -[1.5k]--Gate
*/

#define DETECT 2  //zero cross detect
#define GATE 9    //TRIAC gate
#define PULSE 4   //trigger pulse width (counts)
int i=483;

void setup(){

  // set up pins
  pinMode(DETECT, INPUT);     //zero cross detect
  digitalWrite(DETECT, HIGH); //enable pull-up resistor
  pinMode(GATE, OUTPUT);      //TRIAC gate control

  // set up Timer1 
  //(see ATMEGA 328 data sheet pg 134 for more details)
  OCR1A = 100;      //initialize the comparator
  TIMSK1 = 0x03;    //enable comparator A and overflow interrupts
  TCCR1A = 0x00;    //timer control registers set for
  TCCR1B = 0x00;    //normal operation, timer disabled


  // set up zero crossing interrupt
  attachInterrupt(0,zeroCrossingInterrupt, RISING);    
    //IRQ0 is pin 2. Call zeroCrossingInterrupt 
    //on rising signal

}  

//Interrupt Service Routines

void zeroCrossingInterrupt(){ //zero cross detect   
  TCCR1B=0x04; //start timer with divide by 256 input
  TCNT1 = 0;   //reset timer - count from zero
}

ISR(TIMER1_COMPA_vect){ //comparator match
  digitalWrite(GATE,HIGH);  //set TRIAC gate to high
  TCNT1 = 65536-PULSE;      //trigger pulse width
}

ISR(TIMER1_OVF_vect){ //timer1 overflow
  digitalWrite(GATE,LOW); //turn off TRIAC gate
  TCCR1B = 0x00;          //disable timer stopd unintended triggers
}

void loop(){ // sample code to exercise the circuit

i--;
OCR1A = i;     //set the compare register brightness desired.
if (i<65){i=483;}                      
delay(15);                             

}
