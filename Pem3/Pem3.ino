/*
 * PEM (Photo Electro Multiplier)
 * data acquestion software
 * amp1 correction: 347.1305541*(u-(-1.251621925));
 * TlnT:  3345.8231*(1.10036274781-u);
 * external transistor correction:   T*.9612-15.29;
********************************************************************************
 */

#include <easyModbus.h>
#include <AD7714.h>
#include <EEPROM.h>
#include <setjmp.h>

enum {pwm_pin = 3, adc_cs=10 };
  static uint16_t ncal = 0;
  static uint8_t fsm = 0;

jmp_buf cpget;

#define MCLK 2457600
#define SLAVE_ID 2

#define BUFFER_SIZE 72
unsigned char frame[BUFFER_SIZE];

//3.5 symbol time: 4 for 9600, of 1.75ms for baud > 19200
#define PACKET_END_MS  4

AD7714 adc(2.5,adc_cs);

int   errorCount;
int   controller;     // temperature regulation
int   measured;
float gett;
float geti;
float ipart;
float tprev;
float regpwm;
float tt;
int32_t fils[4]; // digital filter

#define CALFILTER 32

enum { pRegt=0, pSP, pTI, pTcorr_add, pTcorr_mul, pTconv_a, pTconv_b,
    pAmp_offs, pAmp_trans, PARMSF,
    pKzs1=0, pKfs1, pKzs2, pKfs2, pBaud, PARMSL,
    pDataformat=0, pNcal, PARMSW
     };

#define PARAMSZE (sizeof(struct paramstore)) 
#define PARAMWORDS ((PARAMSZE+1)/2) 
struct paramstore  {
    uint16_t crc;
    float f[PARMSF];
    uint32_t l[PARMSL];
    uint16_t w[PARMSW];
} ee;

// stored parameters thermo
// FLOAT
//0:regt Terr = regt - T
//1:kp;     // P = kp*Terr
//2:ki;     // I = ki*DT*accum(Terr) = accum(Terr*ki*DT)
//3,4:tcorr_add, tcorr_mul; // T = Tconv*tcorr_mul + tcorr_add;
//5,6:tconv_a, tconv_b;   // TlnT = tconva(tconv_b-adct);
//7:amp_offs; // u = adci-offs
//8:amp_trans; // i_pem = trans*u
// stored parameters 
// LONG
//0:kZS1;  // ZS = ZSself + kZS
//1:kFS1;  // FS = FSself + FSself*kFS/1000
//2:kZS2
//3:kZS2
//4:baudrate: 9600
// short
//0:dataformat,0802: 08-bits/per byte, 0-noparity, 2-twostops
//1:Ncal: recalibration period in samples

void default_param()
{
  ee.f[pRegt] = -65;
  ee.f[pSP] = 25; // degree/fscale
  ee.f[pTI] = 72; // sec

  ee.f[pTcorr_mul] = 0.9612;
  ee.f[pTcorr_add] = -15.29;

  ee.f[pTconv_a] = 3345.8231;
  ee.f[pTconv_b] = 1.10036274781;
  
  ee.f[pAmp_offs] = -1.251621925;
  ee.f[pAmp_trans] = 347.1305541;

  ee.l[pKzs1] = 0;
  ee.l[pKfs1] = 0;
  ee.l[pKzs2] = 0;
  ee.l[pKfs2] = 0;
  ee.l[pBaud] = 9600;
  
  ee.w[pDataformat] = 802;
  ee.w[pNcal] = 20;
}

void store_param()
{
    int i;

    ee.crc = calculateCRC(2+(uint8_t *)&ee, PARAMSZE-2);
    for(i=0; i < PARAMSZE; i++)
      EEPROM.write(i, ((uint8_t *)&ee)[i]);
}

uint8_t load_param()
{
    int i;
    uint16_t c1;
    struct paramstore tee;

    for(i=0; i < PARAMSZE; i++)
      ((uint8_t *)&tee)[i] = EEPROM.read(i);

    c1= calculateCRC(2+(uint8_t *)&tee, PARAMSZE-2);
    if (tee.crc != c1)
        return 0;
    memmove(&ee,&tee,PARAMSZE);
    return 1;
}

uint8_t serial_config(uint16_t xc)
{
    uint8_t bits, par, stops;
    stops = xc % 10;
    xc/=10;
    par   = xc %10;
    xc/=10;
    bits  = xc %100;
    xc/=100;
    if (xc != 0) return SERIAL_8N2;
    if (bits < 5 || bits >8) return SERIAL_8N2;
    if (par > 2) return SERIAL_8N2;
    if (stops < 1 || stops > 2) return SERIAL_8N2;
    bits=(bits-5)<<1;
    if (par==2) par = 0x20; else if (par==1) par = 0x30;
    if (stops=2) stops = 0x08; else stops=0;
    return bits|par|stops;
}

double TlnT(double T) { return (T <= 0) ? 0 : T*log(T); }
double stepTlnT(double T, double TlnT) { return -(T*log(T)-TlnT)/(1+log(T)); }
double invTlnT(double TlnT) {
double  T = 200; T += stepTlnT(T, TlnT); 
T += stepTlnT(T, TlnT); T += stepTlnT(T, TlnT); return T; }

void taloop(void)
{
  tprev = tt;
  tt = millis();
  adc.fsync(AD7714::CHN_12);
  gett = adc.read(AD7714::CHN_12);
  gett = ee.f[pTconv_a]*(ee.f[pTconv_b]-gett); //TlnT
  gett = invTlnT(gett) - 273.15;
  gett = ee.f[pTcorr_mul]*gett + ee.f[pTcorr_add]; //correction
}

void ialoop(void)
{
  adc.fsync(AD7714::CHN_56);
  geti = adc.read(AD7714::CHN_56);
  geti = ee.f[pAmp_trans]*(geti - ee.f[pAmp_offs]); // PEM current
}

/*
 *  ADDRESS MAP: from 300 to 444 (128+16=144 adresses)
 *    address MOD 8 - ADC channel
 * 300-307: CMM
 * 308-315: MODE
 * 316-323: FILTH
 * 324-331: FILTL
 *    address MOD 16 - ADC channel + low/high
 * 332-347: DATA, 16bit-register pair
 * 348-363: DATA, FSYNC, 16bit-register pair
 *    address MOD 32 - ADC channel + low/high + offset/gain
 * 364-395: OFFSET/GAIN, 16bit-register quad
 * 364:ch16 368:ch26 372:ch36 376:ch46 380:ch12 384:ch34 388:ch56 392:ch66
 * 396-427: OFFSET/GAIN, calibration start, 16bit-register quad
            OFFSET-LOW, OFFSET-HIGH, GAIN-LOW, GAIN-HIGH
 * 428-443: DATA, FSYNC, CONVERTED, float
 *          428:ch16, 430:ch26, 432:ch36, 434:ch46, 436:ch12, 
 *          438:ch34, 440:ch56, 442:ch66
 *
 * FSYNC, CALIBRATION initated @ read low-part
 */
int reg_adcget(uint16_t addr, uint16_t *val)
{
  static uint16_t tempor = 0;
  static uint16_t prevadr= 0;

  uint8_t reg, hi, part;
  uint16_t ch, subadr;
  union {
    float f;
    uint32_t l;
    uint16_t w[2];
  } data;

  if (addr < 300)
    return 0;
  else if (addr < 332) { // 300-332 : 32 : 4x8
    subadr = addr - 300;
    ch = subadr & 0x7;    // addr MOD 8
    reg = (subadr >> 3) & 0x3; // addr DIV 8
    reg = reg << 4;
    *val = adc.command(ch | AD7714::REG_RD | reg, 0xFF);
    prevadr = addr;
  } else if (addr < 364) { // 332-364 : 32 : 2x8x2
    subadr = addr - 332;
    hi = subadr & 1;
    ch = (subadr >> 1) & 0x7;
    part = subadr>>4;
    if (hi && addr-1 == prevadr) {
      *val = tempor;
    } else {
      if (part) adc.fsync(ch);
      adc.wait(ch);
      if (adc.adc_filth & AD7714::WL24)
	    data.l = adc.read24(AD7714::REG_DATA|ch|AD7714::REG_RD);
      else
	    data.l = adc.read16(AD7714::REG_DATA|ch|AD7714::REG_RD);
      tempor = data.w[1];
      *val = data.w[0];
    }
    prevadr = addr;
  } else if (addr < 428) { // 364-428 : 64 : 2x8x2x2
    subadr = addr - 364;
    hi = subadr & 1;
    ch = (subadr >> 2) & 0x7;
    reg = (subadr & 0x2) ? AD7714::REG_GAIN : AD7714::REG_OFFSET;
    part = (subadr>>5) & 1;
    if (hi && addr-1 == prevadr) {
      *val = tempor;
    } else {
      if (part) { // start calibration only if pair mode and 
                  // read offset(low) register
        if ( ((adc.adc_mode == AD7714::MODE_SELF_CAL ||
               adc.adc_mode == AD7714::MODE_SYS_OFFSET_CAL) 
               && (reg == AD7714::REG_OFFSET)
              )||( adc.adc_mode != AD7714::MODE_SELF_CAL &&
                  adc.adc_mode != AD7714::MODE_SYS_OFFSET_CAL) ) {
          adc.command(AD7714::REG_MODE|ch|AD7714::REG_WR,adc.adc_mode);
          adc.wait(ch); 
          ncal++;
        }
      }
      data.l = adc.read24(reg|ch|AD7714::REG_RD);
      tempor=data.w[1];
      *val=data.w[0];
    }
    prevadr = addr;
  } else if (addr < 444) { // 428-444 : 16 : 8x2
    subadr = addr - 428;
    hi = subadr & 1;
    ch = (subadr >> 1) & 0x7;
    if (hi && addr-1 == prevadr) {
      *val = tempor;
    } else {
      switch(ch) {
        case AD7714::CHN_56:
          ialoop();
          data.f = geti;
	  break;
        case AD7714::CHN_12:
          taloop();
          data.f = gett;
	  break;
        default:
          adc.fsync(ch);
          adc.wait(ch);
          data.f = adc.read(ch);
	  break;
      }
      tempor = data.w[1];
      *val = data.w[0];
    }
    prevadr = addr;
  } else
    return 0;
  return 1;
}

int reg_adcput(uint16_t addr, uint16_t val)
{
  static uint16_t tempor = 0;
  static uint16_t prevadr= 0;

  uint8_t reg, hi;
  uint16_t ch, subadr;
  union {
    uint32_t l;
    uint16_t w[2];
  } data;

  if (addr < 300)
    return 0;
  else if (addr < 332) { // 300-332 : 32 : 4x8
    subadr = addr - 300;
    ch = subadr & 0x7;    // addr MOD 8
    reg = (subadr >> 3) & 0x3; // addr DIV 8
    reg = reg << 4;
    adc.command(ch | AD7714::REG_WR | reg, val & 0xFF);
    prevadr = addr;
  } else if (addr < 364) { // 332-364 : 32 : 2x8x2
    subadr = addr - 332;
    hi = subadr & 1;
    ch = (subadr >> 1) & 0x7;
    if (hi && addr-1 == prevadr) {
      data.w[1]=val;
      data.w[0] = tempor;
      adc.load24(AD7714::REG_DATA|ch|AD7714::REG_WR,data.l);
    } else {
      tempor=val;
    }
    prevadr = addr;
  } else if (addr < 428) { // 364-428 : 64 : 2x8x2x2
    subadr = addr - 364;
    hi = subadr & 1;
    ch = (subadr >> 2) & 0x7;
    reg = (subadr & 0x2) ? AD7714::REG_GAIN : AD7714::REG_OFFSET;
    if (hi && addr-1 == prevadr) {
      data.w[1]=val;
      data.w[0] = tempor;
      adc.load24(reg|ch|AD7714::REG_WR, data.l);
    } else {
      tempor = val;
    }
    prevadr = addr;
  } else
    return 0;
  return 1;
}

/*
 * parameters
 *  ADDRESS MAP: from 500 to 500+PARAMWORDS
 *  500: crc: default settings 0xFF00; 0xFF55-load EEPROM settings; 0x55FF-store
 *
 *  501-502: regulated temperature, float
 *  ....
 *
 */
 
int reg_parmget(uint16_t addr, uint16_t *val)
{
  if (addr < 500)
    return 0;
  else if (addr < 500+PARAMWORDS) {
    *val = ((uint16_t*)&ee)[addr - 500];
  } else
     return 0;
  return 1;
}

int reg_parmput(uint16_t addr, uint16_t val)
{
  if (addr < 500)
    return 0;
  else if (addr < 500+PARAMWORDS) {
    ((uint16_t*)&ee)[addr - 500] = val;
    if (addr == 500) {
      if (val==0x55FF) store_param(); // write CRC initate storing
      if (val==0xFF00) default_param();
      if (val==0xFF55) load_param();
    }
  } else
     return 0;
  return 1; 
}

/*
 * operational parameters
 *  ADDRESS MAP: from 0 to 17
 *  0: measurement loop on [1]
 *  1: reserved
 *  2: error count
 *  3: reserver
 *  4,5: proportional        (direct pwm control
 *  6,7: integral part 
 *  8,9: delta-t             (freerun cycle)
 *  10: data wait register   (wait cycle)
 *  11: data ready register  (since lasr read 12,13)
 * 12,13: temperature Float
 * 14,15: PEM current Float
 * 16,17: miliiseconds Long
 */

int reg_operget(uint16_t addr, uint16_t *val)
{
  static uint16_t tempor = 0;
  static uint16_t prevadr= 0;

  union {
    float f;
    uint32_t l;
    uint16_t w[2];
  } data;

  switch(addr) {
  case 0:*val = controller; break;
  case 1:*val = ncal; break;
  case 2:*val = errorCount; break;
  case 3:*val = fsm; break;

  case 4:
      data.f = regpwm;
      *val = data.w[0];
      tempor = data.w[1]; break;
  case 5:*val = tempor; break;

  case 6:
      data.f = ipart;
      *val = data.w[0];
      tempor = data.w[1]; break;
  case 7:*val = tempor; break;

  case 8:
      data.f = (tt-tprev)/1000.0;
      *val = data.w[0];
      tempor = data.w[1]; break;
  case 9:*val = tempor; break;
  
  case 10:
     *val = measured; break;
  case 11:
     *val = measured; break;
 
  case 12:
      data.f = gett;
      *val = data.w[0];
      tempor = data.w[1];
      measured = 0;
      break;
  case 13:*val = tempor; break;
  case 14:
      data.f = geti;
      *val = data.w[0];
      tempor = data.w[1];
      measured = 0;
      break;
  case 15:*val = tempor; break;
  case 16:
      data.l = millis();
      *val = data.w[0];
      tempor = data.w[1];
      break;
  case 17:*val = tempor; break;
  default:   return 0;
  }
  return 1;
}

int reg_operput(uint16_t addr, uint16_t val)
{
  static uint16_t tempor = 0;
  static uint16_t prevadr= 0;

  union {
    float f;
    uint32_t l;
    uint16_t w[2];
  } data;

  switch(addr) {
  case 0:controller=val; break;
  case 1:
    break;
  case 2:errorCount=val; break;

  case 4: tempor = val; break;
  case 5:
      data.w[0] = tempor;
      data.w[1] = val;
      regpwm = data.f;
      break;

  case 6: tempor = val; break;
  case 7:
      data.w[0] = tempor;
      data.w[1] = val;
      ipart = data.f;
      break;
  default:   return 0;
  }
  return 1;
}

int modbus_my3(unsigned char *frame, uint8_t *len)
{
  uint16_t startingAddress = ((frame[2] << 8) | frame[3]);
  uint16_t no_of_registers = ((frame[4] << 8) | frame[5]);
  uint8_t *address;
  uint16_t crc16;
  uint16_t v;

  if (frame[0]==0) return -1; // broadcast has no answer

  if (startingAddress > 600)
    return modbus_exception(frame, len, 2); // 2 ILLEGAL DATA ADDRESS

  if (startingAddress + no_of_registers > 600)
    return modbus_exception(frame, len, 3); // 3 ILLEGAL DATA VALUE

 // ID, function, noOfBytes, (dataLo + dataHi)*number of registers, crcLo, crcHi
  address = frame + 3; // PDU starts at the 4th byte
  uint8_t noOfBytes = 0; 
  for(; no_of_registers--; startingAddress++, noOfBytes+=2) {
    if (reg_adcget(startingAddress, &v)) { }
    else if (reg_operget(startingAddress, &v)) { }
    else if (reg_parmget(startingAddress, &v)) { } 
    else break;
    *address++ = (v >> 8) & 0xFF;
    *address++ = v & 0xFF;
  }

  frame[2] = noOfBytes;
  uint8_t responseFrameSize = 5 + noOfBytes; 
							
  crc16 = calculateCRC(frame, responseFrameSize - 2);
  frame[responseFrameSize - 2] = crc16 & 0xFF;
  frame[responseFrameSize - 1] = crc16 >> 8; // split crc into 2 bytes
  *len = responseFrameSize;
  return 0; // zero errors
}	

int modbus_my16(unsigned char *frame, uint8_t *len)
{
  uint16_t startingAddress = ((frame[2] << 8) | frame[3]);
  uint16_t no_of_registers = ((frame[4] << 8) | frame[5]);
  uint8_t *address;
  uint16_t crc16;
  uint16_t v;

  if (startingAddress > 600)
    return modbus_exception(frame, len, 2); // 2 ILLEGAL DATA ADDRESS

  if (startingAddress + no_of_registers > 600)
    return modbus_exception(frame, len, 3); // 3 ILLEGAL DATA VALUE

  address = frame + 7; // start at the 8th byte in the frame
  uint16_t noOfWrites = 0; 
  for(; no_of_registers--; startingAddress++, noOfWrites++) {
    v = ((*address) << 8) | *(address + 1);  address+=2;
    if (reg_adcput(startingAddress, v)) {
    } else if (reg_operput(startingAddress, v)) {
    } else if (reg_parmput(startingAddress, v)) {
    } else break;
  }

  if (frame[0]==0) return -1; // broadcast has no answer

  frame[4] = noOfWrites >> 8; 
  frame[5] = noOfWrites & 0xFF;
  crc16 = calculateCRC(frame, 6); 
  frame[6] = crc16 & 0xFF;
  frame[7] = crc16 >> 8; 
  *len = 8;
  return 0;
}

int modbus_my6(unsigned char *frame, uint8_t *len)
{
  uint16_t startingAddress = ((frame[2] << 8) | frame[3]);
  uint16_t v;

  if (startingAddress > 600)
    return modbus_exception(frame, len, 2); // 2 ILLEGAL DATA ADDRESS

  v = ((frame[4]) << 8) | frame[5];
  if (reg_adcput(startingAddress, v)) {
  } else if (reg_operput(startingAddress, v)) {
  } else if (reg_parmput(startingAddress, v)) {
  } else return modbus_exception(frame, len, 2); // 2 ILLEGAL DATA ADDRESS

  if (frame[0]==0) return -1; // broadcast has no answer

  // *len unchanged, reply as is.
  return 0;
}

int modbus_my(HardwareSerial *Port, unsigned char *frame, uint8_t len,
	uint8_t id, int *errors)
{
  int  rc;
  if (frame[0] != id && frame[0] != 0) return 0;

  if (frame[1]&0x80) 
     rc = -1;
  else if (frame[1]==3) 
     rc = modbus_my3(frame, &len);
  else if (frame[1]==16) 
     rc = modbus_my16(frame, &len);
  else if (frame[1]==6) 
     rc = modbus_my6(frame, &len);
  else
     rc = modbus_exception(frame, &len, 1); // 1:illegal function

  if (rc < 0)  return 0;  

  (*errors) += rc;

  for (rc = 0; rc < len; rc++)
    (*Port).write(frame[rc]);
  (*Port).flush();
	
  // allow a frame delay to indicate end of transmission
  delay(3); 
}


void trloop()
{
    float er;

    // controller error
    er = (ee.f[pRegt]- gett);

    // regulation action
    regpwm = er/ee.f[pSP] + ipart;
    if (regpwm < 0) regpwm=0;
    else if (regpwm > 1) regpwm=1;
    else {  // integral part adjust, if signal in range
      ipart += (tt-tprev)*0.001*er/ee.f[pSP]/ee.f[pTI]; // time in ms
      if (ipart < -1) ipart = -1;
      if (ipart > 1) ipart = 1;
    }
    analogWrite(pwm_pin, regpwm*253);
}


uint32_t filtr(uint8_t index, int32_t x)
{
  fils[index] += x - fils[index]/CALFILTER;
  return fils[index]/CALFILTER;
}

void cal_correct(uint8_t cal_thing)
{
  uint8_t i;
  uint8_t r;
  int32_t c1;

  switch(cal_thing&0x7) {
    default:
    case AD7714::CHN_12: i=0;
    case AD7714::CHN_56: i=2;
  }
  if(cal_thing&0xE0==AD7714::MODE_SELF_FS_CAL) {
    i++;
    r = AD7714::REG_GAIN|(cal_thing&0x7);
  } else 
    r = AD7714::REG_OFFSET|(cal_thing&0x7);

 // read mode
  adc.command((cal_thing&0x7)|AD7714::REG_RD|AD7714::REG_MODE, 0xFF);
  c1=filtr(i, adc.read24(r|AD7714::REG_RD));
  if (i & 1) // fullscale
     c1 += (float)ee.l[i]*(float)c1*0.001; // stored value is*1000
  else  // zeroscale
     c1 += ee.l[i];
  adc.load24(r|AD7714::REG_WR, c1);
}


int adc_loop()
{
  static uint8_t xcal = AD7714::CHN_12|AD7714::MODE_SELF_ZS_CAL;
  static uint32_t toux;

  if (!controller) return 0;  

  switch (fsm) {
  default: fsm = 0;
  case 0:
    adc.reset();  // spi interface reset, send 0xFFFFFFFF
    tprev = tt;
    tt = millis();
    fsm++;
   case 1:
    toux = millis();
    adc.fsync(AD7714::CHN_12); // wait 3T
    toux+=adc.t9conv(MCLK)/3;
    fsm++;
    break;
   case 2:
    if (toux > millis()) break;
    adc.wait(AD7714::CHN_12);
    fsm++;
   case 3:
    gett = adc.conv(adc.read24(AD7714::CHN_12|AD7714::REG_DATA|AD7714::REG_RD));
    gett = ee.f[pTconv_a]*(ee.f[pTconv_b]-gett); //TlnT
    gett = invTlnT(gett) - 273.15;
    gett = ee.f[pTcorr_mul]*gett + ee.f[pTcorr_add]; //correction
    fsm++;
   case 4:
    toux = millis();
    adc.fsync(AD7714::CHN_56);
    toux+=adc.t9conv(MCLK)/3;
    fsm++;
    break;
   case 5:
    if (toux > millis()) break;
    adc.wait(AD7714::CHN_56);
    fsm++;
   case 6:
    geti = adc.conv(adc.read24(AD7714::CHN_56|AD7714::REG_DATA|AD7714::REG_RD));
    geti = ee.f[pAmp_trans]*(geti - ee.f[pAmp_offs]); // PEM current

    if(ee.w[pNcal])
      ncal++;
    else
      ncal=0;

    if (ncal <= ee.w[pNcal])
      fsm = 0; 
    else
      fsm ++;
    return 1;
   case 7:
    toux = millis();
    adc.command((xcal&0x7)|AD7714::REG_WR|AD7714::REG_MODE, (xcal&0xE0));
            // assume gain=1
    toux+=adc.t9conv(MCLK)/3; // 3T self calibration
    fsm++;
    break;
   case 8:
    if (toux > millis()) break;
    adc.wait(xcal&0x7);
    cal_correct(xcal);
    switch(xcal){
      default:
      case AD7714::CHN_12|AD7714::MODE_SELF_ZS_CAL:
          xcal=AD7714::CHN_12|AD7714::MODE_SELF_FS_CAL; break;
      case AD7714::CHN_12|AD7714::MODE_SELF_FS_CAL:
          xcal=AD7714::CHN_56|AD7714::MODE_SELF_ZS_CAL; break;
      case AD7714::CHN_56|AD7714::MODE_SELF_ZS_CAL:
          xcal=AD7714::CHN_56|AD7714::MODE_SELF_FS_CAL; break;
      case AD7714::CHN_56|AD7714::MODE_SELF_FS_CAL:
          xcal=AD7714::CHN_12|AD7714::MODE_SELF_ZS_CAL; break;
    }
    ncal=0;
    fsm=0;
    break;
  }
  return 0;
}

void setup()
{
  pinMode(pwm_pin, OUTPUT); // pin
  analogWrite(pwm_pin, 0);

  default_param();
  load_param();

  Serial.begin(ee.l[pBaud], serial_config(ee.w[pDataformat]));
//  Serial.begin(9600, SERIAL_8N2);
  errorCount = 0;

  ipart = 0;
  gett = 25;
  geti = 0;
  controller = 1; // regulator on
  regpwm = 0;
  measured = 1;

  adc.reset();  // 1920:  sr=10Hz
  adc.init(0, AD7714::BIPOLAR, AD7714::GAIN_1, 1920, AD7714::NOCALIBRATE);
  adc.self_calibrate(AD7714::CHN_12);
  fils[0]=adc.read24(AD7714::CHN_12|AD7714::REG_RD|AD7714::REG_OFFSET)*CALFILTER;
  fils[1]=adc.read24(AD7714::CHN_12|AD7714::REG_RD|AD7714::REG_GAIN)*CALFILTER;
  adc.self_calibrate(AD7714::CHN_56);
  fils[2]=adc.read24(AD7714::CHN_56|AD7714::REG_RD|AD7714::REG_OFFSET)*CALFILTER;
  fils[3]=adc.read24(AD7714::CHN_56|AD7714::REG_RD|AD7714::REG_GAIN)*CALFILTER;
  adc.command(AD7714::REG_MODE|AD7714::REG_RD, 0xFF);  // update mode register
  tt = tprev = millis();
}

void loop()
{
  uint8_t len;

  len = BUFFER_SIZE;
  if (modbus_read(&Serial, frame, &len, PACKET_END_MS, &errorCount))
      modbus_my(&Serial, frame, len, 
	SLAVE_ID, &errorCount); // TXEnable can be here
  if (controller && adc_loop()) {
      trloop();
      measured = 1;
  }
}


