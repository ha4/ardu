
#include <Wire.h> 

static uint8_t rawADC[6];
int16_t rollADC, pitchADC, yawADC;

#define GYRO_ORIENTATION(X, Y, Z) {rollADC = X; pitchADC = Y; yawADC = Z;}
#define ACC_ORIENTATION(X, Y, Z)  {rollADC = -Y; pitchADC = X; yawADC = Z;}
#define MAG_ORIENTATION(X, Y, Z)  {rollADC = -Y; pitchADC = X; yawADC = Z;}

void i2c_getSixRawADC(uint8_t add, uint8_t reg) {
  Wire.beginTransmission(add);
  Wire.write(reg);
  Wire.requestFrom(add, (byte)6);
  while (Wire.available() < 6);
  for(int i = 0; i < 6; i++) rawADC[i] = Wire.read();
  Wire.endTransmission();
}

void i2c_writeReg(uint8_t add, uint8_t reg, uint8_t val)
{
  Wire.beginTransmission(add);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}


#define L3G4200D_ADDRESS 0x69
void Gyro_init()
{
  delay(100);
  i2c_writeReg(L3G4200D_ADDRESS ,0x20 ,0x8F ); // CTRL_REG1   400Hz ODR, 20hz filter, run!
  delay(5);
  i2c_writeReg(L3G4200D_ADDRESS ,0x24 ,0x02 ); // CTRL_REG5   low pass filter enable
  delay(5);
  i2c_writeReg(L3G4200D_ADDRESS ,0x23 ,0x30); // CTRL_REG4 Select 2000dps
}

void Gyro_getADC()
{
  i2c_getSixRawADC(L3G4200D_ADDRESS, 0x80|0x28);

  GYRO_ORIENTATION( ((rawADC[1]<<8) | rawADC[0])>>2,
                    ((rawADC[3]<<8) | rawADC[2])>>2,
                    (( rawADC[5]<<8) | rawADC[4])>>2  );
}


#define MAG_ADDRESS 0x1E
#define MAG_DATA_REGISTER 0x03

void Mag_getADC()
{
  i2c_getSixRawADC(MAG_ADDRESS,MAG_DATA_REGISTER);
  MAG_ORIENTATION( ((rawADC[0]<<8) | rawADC[1])>>1,
                   ((rawADC[2]<<8) | rawADC[3])>>1,
                   ((rawADC[4]<<8) | rawADC[5])>>1); // high first, 12 bit
}
 
void Mag_init()
{ 
  delay(100);

  i2c_writeReg(MAG_ADDRESS ,0x00 ,0x10 ); //Configuration Register A  -- 0 00 100 00  disable TEMP; output rate: 15Hz
  i2c_writeReg(MAG_ADDRESS ,0x01 ,0x20 ); //Configuration Register B  -- 001 00000    configuration gain 1.3Ga
  i2c_writeReg(MAG_ADDRESS ,0x02 ,0x00 ); //Mode register             -- 000000 00    continuous Conversion Mode
}


#define LSM303ACC_ADDRESS 0x19
void ACC_init ()
{
  delay(10);
  i2c_writeReg(LSM303ACC_ADDRESS, 0x20, 0x27); // 0010 0 111     datarate 10Hz, Lowpower disable, XYZ enable
  i2c_writeReg(LSM303ACC_ADDRESS, 0x23, 0x30); // 0 0 11 0 00 0  continus update, little endian, +-16G/FullScale, hires-mode
  i2c_writeReg(LSM303ACC_ADDRESS, 0x21, 0x00); // 00 00 0 0 00   hipas bypassed
}

void ACC_getADC ()
{
  i2c_getSixRawADC(LSM303ACC_ADDRESS,0x28 | 0x80); // 1xxxxxxx - multiply read

  ACC_ORIENTATION( ((rawADC[1]<<8) | rawADC[0])>>2,
                   ((rawADC[3]<<8) | rawADC[2])>>2,
                   ((rawADC[5]<<8) | rawADC[4])>>2 );
}

void setup() 
{
  Wire.begin();
  Serial.begin(115200);
  Gyro_init();
  Mag_init();
  ACC_init();
}

void printadc()
{
  Serial.print(rollADC);
  Serial.print(" ");
  Serial.print(pitchADC);
  Serial.print(" ");
  Serial.print(yawADC);
}

void loop() 
{
  Gyro_getADC(); printadc();
  Serial.print(" ");
//  Mag_getADC(); printadc();  Serial.print(" ");
//  Serial.print(" ");
  ACC_getADC(); printadc();
  Serial.println("");

  delay(20);
}

