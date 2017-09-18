#include <SPI.h>

//
// uses an nRF24L01p connected to an Arduino
// 
// Cables are:
//     SS       -> 10
//     MOSI     -> 11
//     MISO     -> 12
//     SCK      -> 13
// 
// and CE       ->  9
//
//  2  4  6  8  front view
//  1  3  5  7
//
// GND CE CK MISO
//  1  3  5  7  edge view
//  ----------
//  2  4  6  8 INT 
// VCC CS MOSI 

enum pins { CE =  9, CS = 10, pinLED = 3} ;


static uint32_t tmr1;
static uint32_t tmr1_wait;
static uint32_t tmr2;
static uint32_t tmr2_wait;

byte tmr1_chk(uint32_t t) // call with micros()
{
    if (tmr1_wait==0xFFFFFFFF) { tmr1 = t; return 0; }
    if (tmr1_wait==0) { tmr1 = t; return 1; }
    if (t - tmr1 < tmr1_wait)  return 0;
    tmr1 = t;
    return 1;
}
void tmr1_set(uint32_t dt) { tmr1_wait=dt; }
void tmr1_start(uint32_t now) { tmr1 = now; }
void tmr1_stop() { tmr1_set(0xFFFFFFFF); }

byte tmr2_chk(uint32_t t) // call with millis()
{
    if (tmr2_wait==0xFFFFFFFF) { tmr2 = t; return 0; }
    if (tmr2_wait==0) { tmr2 = t; return 1; }
    if (t - tmr2 < tmr2_wait)  return 0;
    tmr2 = t;
    return 1;
}
void tmr2_set(uint32_t dt) { tmr2_wait=dt; }
void tmr2_start(uint32_t now) { tmr2 = now; }
void tmr2_stop() { tmr2_set(0xFFFFFFFF); }

enum TXRX_State {
    TXRX_OFF,
    TX_EN,
    RX_EN,
} ;

enum TxPower {
    TXPOWER_100uW,
    TXPOWER_300uW,
    TXPOWER_1mW,
    TXPOWER_3mW,
    TXPOWER_10mW,
    TXPOWER_30mW,
    TXPOWER_100mW,
    TXPOWER_150mW,
    TXPOWER_LAST,
};

static byte rf_setup;

// Register map
enum {
    NRF24L01_00_CONFIG      = 0x00,
    NRF24L01_01_EN_AA       = 0x01,
    NRF24L01_02_EN_RXADDR   = 0x02,
    NRF24L01_03_SETUP_AW    = 0x03,
    NRF24L01_04_SETUP_RETR  = 0x04,
    NRF24L01_05_RF_CH       = 0x05,
    NRF24L01_06_RF_SETUP    = 0x06,
    NRF24L01_07_STATUS      = 0x07,
    NRF24L01_08_OBSERVE_TX  = 0x08,
    NRF24L01_09_CD          = 0x09,
    NRF24L01_0A_RX_ADDR_P0  = 0x0A,
    NRF24L01_0B_RX_ADDR_P1  = 0x0B,
    NRF24L01_0C_RX_ADDR_P2  = 0x0C,
    NRF24L01_0D_RX_ADDR_P3  = 0x0D,
    NRF24L01_0E_RX_ADDR_P4  = 0x0E,
    NRF24L01_0F_RX_ADDR_P5  = 0x0F,
    NRF24L01_10_TX_ADDR     = 0x10,
    NRF24L01_11_RX_PW_P0    = 0x11,
    NRF24L01_12_RX_PW_P1    = 0x12,
    NRF24L01_13_RX_PW_P2    = 0x13,
    NRF24L01_14_RX_PW_P3    = 0x14,
    NRF24L01_15_RX_PW_P4    = 0x15,
    NRF24L01_16_RX_PW_P5    = 0x16,
    NRF24L01_17_FIFO_STATUS = 0x17,
    NRF24L01_1C_DYNPD       = 0x1C,
    NRF24L01_1D_FEATURE     = 0x1D,
    //Instructions
    NRF24L01_61_RX_PAYLOAD  = 0x61,
    NRF24L01_A0_TX_PAYLOAD  = 0xA0,
    NRF24L01_E1_FLUSH_TX    = 0xE1,
    NRF24L01_E2_FLUSH_RX    = 0xE2,
    NRF24L01_E3_REUSE_TX_PL = 0xE3,
    NRF24L01_50_ACTIVATE    = 0x50,
    NRF24L01_60_R_RX_PL_WID = 0x60,
    NRF24L01_B0_TX_PYLD_NOACK = 0xB0,
    NRF24L01_FF_NOP         = 0xFF,
    NRF24L01_A8_W_ACK_PAYLOAD0 = 0xA8,
    NRF24L01_A8_W_ACK_PAYLOAD1 = 0xA9,
    NRF24L01_A8_W_ACK_PAYLOAD2 = 0xAA,
    NRF24L01_A8_W_ACK_PAYLOAD3 = 0xAB,
    NRF24L01_A8_W_ACK_PAYLOAD4 = 0xAC,
    NRF24L01_A8_W_ACK_PAYLOAD5 = 0xAD,
};

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
#define NOP           0xFF

// Bit mnemonics
enum {
    NRF24L01_00_MASK_RX_DR  = 6,
    NRF24L01_00_MASK_TX_DS  = 5,
    NRF24L01_00_MASK_MAX_RT = 4,
    NRF24L01_00_EN_CRC      = 3,
    NRF24L01_00_CRCO        = 2,
    NRF24L01_00_PWR_UP      = 1,
    NRF24L01_00_PRIM_RX     = 0,

    NRF24L01_07_RX_DR       = 6,
    NRF24L01_07_TX_DS       = 5,
    NRF24L01_07_MAX_RT      = 4,

    NRF2401_1D_EN_DYN_ACK   = 0,
    NRF2401_1D_EN_ACK_PAY   = 1,
    NRF2401_1D_EN_DPL       = 2,
};

// Bitrates
enum {
    NRF24L01_BR_1M = 0,
    NRF24L01_BR_2M,
    NRF24L01_BR_250K,
    NRF24L01_BR_RSVD
};
    
// enable RX   CE=1
void enable(byte y)  { digitalWrite(CE, y); }

byte NRF24L01_WriteReg(byte reg, byte data)
{
  digitalWrite(CS, 0);
  byte res = SPI.transfer(W_REGISTER | (REGISTER_MASK & reg));
  SPI.transfer(data);
  digitalWrite(CS, 1);
  return res;
}

byte NRF24L01_WriteRegisterMulti(byte reg, const byte data[], byte length)
{
  digitalWrite(CS, 0);
  byte res = SPI.transfer(W_REGISTER | (REGISTER_MASK & reg));
  for (byte i = 0; i < length; i++)
    SPI.transfer(data[i]);
  digitalWrite(CS, 1);
  return res;
}

byte NRF24L01_WritePayload(byte *data, byte length)
{
  digitalWrite(CS, 0);
  byte res = SPI.transfer(W_TX_PAYLOAD);
  for (byte i = 0; i < length; i++)
    SPI.transfer(data[i]);
  digitalWrite(CS, 1);
  return res;
}

byte NRF24L01_ReadReg(byte reg)
{
  digitalWrite(CS, 0);
  SPI.transfer(R_REGISTER | (REGISTER_MASK & reg));
  byte data = SPI.transfer(0xFF);
  digitalWrite(CS, 1);
  return data;
}

byte NRF24L01_ReadRegisterMulti(byte reg, byte data[], byte length)
{
  digitalWrite(CS, 0);
  byte res = SPI.transfer(R_REGISTER | (REGISTER_MASK & reg));
  for(byte i = 0; i < length; i++)
     data[i] = SPI.transfer(0xFF);
  digitalWrite(CS, 1);
  return res;
}

byte NRF24L01_ReadPayload(byte *data, byte length)
{
  digitalWrite(CS, 0);
  byte res = SPI.transfer(R_RX_PAYLOAD);
  for(byte i = 0; i < length; i++)
     data[i] = SPI.transfer(0xFF);
  digitalWrite(CS, 1);
  return res;
}

static byte Strobe(byte state)
{
  digitalWrite(CS, 0);
  byte res = SPI.transfer(state);
  digitalWrite(CS, 1);
  return res;
}

byte NRF24L01_FlushTx() { return Strobe(FLUSH_TX); }
byte NRF24L01_FlushRx() { return Strobe(FLUSH_RX); }

byte NRF24L01_Activate(byte code)
{
  digitalWrite(CS, 0);
  byte res = SPI.transfer(ACTIVATE);
  SPI.transfer(code);
  digitalWrite(CS, 1);
  return res;
}

// Bitrate 0 - 1Mbps, 1 - 2Mbps, 3 - 250K (for nRF24L01+)
byte NRF24L01_SetBitrate(byte bitrate)
{
  // Note that bitrate 250kbps (and bit RF_DR_LOW) is valid only for nRF24L01+. 
  // Bit 0 goes to RF_DR_HIGH, bit 1 - to RF_DR_LOW
  rf_setup = (rf_setup & 0xD7) | ((bitrate & 0x02) << 4) | ((bitrate & 0x01) << 3);
  return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

byte NRF24L01_SetPower(byte power)
{
    byte nrf_power = 0;
    switch(power) {
        case TXPOWER_100uW: nrf_power = 0; break;
        case TXPOWER_300uW: nrf_power = 0; break;
        case TXPOWER_1mW:   nrf_power = 0; break;
        case TXPOWER_3mW:   nrf_power = 1; break;
        case TXPOWER_10mW:  nrf_power = 1; break;
        case TXPOWER_30mW:  nrf_power = 2; break;
        case TXPOWER_100mW: nrf_power = 3; break;
        case TXPOWER_150mW: nrf_power = 3; break;
        default:            nrf_power = 0; break;
    }
    // Power is in range 0..3 for nRF24L01
    rf_setup = (rf_setup & 0xF9) | ((nrf_power & 0x03) << 1);
    return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

void NRF24L01_SetTxRxMode(byte mode)
{
    if(mode == TX_EN) {
        digitalWrite(CE,0);
        NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)    //reset the flag(s)
                                            | (1 << NRF24L01_07_TX_DS)
                                            | (1 << NRF24L01_07_MAX_RT));
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to TX mode
                                            | (1 << NRF24L01_00_CRCO)
                                            | (1 << NRF24L01_00_PWR_UP));
        delayMicroseconds(130);
        digitalWrite(CE,1);
    } else if (mode == RX_EN) {
        digitalWrite(CE,0);
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);        // reset the flag(s)
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);        // switch to RX mode
        NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)    //reset the flag(s)
                                            | (1 << NRF24L01_07_TX_DS)
                                            | (1 << NRF24L01_07_MAX_RT));
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to RX mode
                                            | (1 << NRF24L01_00_CRCO)
                                            | (1 << NRF24L01_00_PWR_UP)
                                            | (1 << NRF24L01_00_PRIM_RX));
        delayMicroseconds(130);
        digitalWrite(CE,1);
    } else {
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)); //PowerDown
        digitalWrite(CE,0);
    }
}

void NRF24L01_Initialize() { rf_setup = 0x0F; }    

int NRF24L01_Reset()
{
  NRF24L01_FlushTx();
  NRF24L01_FlushRx();
  byte status1 = Strobe(NOP);
  byte status2 = NRF24L01_ReadReg(0x07);
  NRF24L01_SetTxRxMode(TXRX_OFF);
  return (status1 == status2 && (status1 & 0x0f) == 0x0e);
}


/*
 *  PROTOCOL PART
 *
 */

#define PAYLOADSIZE 8       // receive data pipes set to this size, but unused
#define MAX_PACKET_SIZE 9   // YD717 packets have 8-byte payload, Syma X4 is 9

#define RF_CHANNEL 0x3C

#define FLAG_FLIP   0x0F
#define FLAG_LIGHT  0x10

enum phase {
    YD717_INIT1 = 0,
    YD717_BIND2,
    YD717_BIND3,
    YD717_DATA
};

enum {
    PKT_PENDING = 0,
    PKT_ACKED,
    PKT_TIMEOUT
};

#define PACKET_PERIOD   8000     // Timeout for callback in uSec, 8ms=8000us for YD717
#define INITIAL_WAIT   50000     // Initial wait before starting callbacks
#define PACKET_CHKTIME   500     // Time to wait if packet not yet acknowledged or timed out    
#define BIND_COUNT        60

static byte rx_tx_addr[5];
static byte phase;
static uint16_t counter;
static uint32_t packet_counter;
static byte tx_power;
static byte packet[MAX_PACKET_SIZE];
static byte throttle, rudder, elevator, aileron, flags;
static byte rudder_trim, elevator_trim, aileron_trim;

static byte packet_ack()
{
    switch (NRF24L01_ReadReg(NRF24L01_07_STATUS) & (_BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT))) {
    case _BV(NRF24L01_07_TX_DS):  return PKT_ACKED;
    case _BV(NRF24L01_07_MAX_RT): return PKT_TIMEOUT;
    default: return PKT_PENDING;
    }
}


static void send_packet(byte bind)
{
    if (bind) {
        packet[0]= rx_tx_addr[0]; // send data phase address in first 4 bytes
        packet[1]= rx_tx_addr[1];
        packet[2]= rx_tx_addr[2];
        packet[3]= rx_tx_addr[3];
        packet[4] = 0x56;
        packet[5] = 0xAA;
        packet[6] = 0x32;
        packet[7] = 0x00;
    } else {
//        read_controls(&throttle, &rudder, &elevator, &aileron, &flags, &rudder_trim, &elevator_trim, &aileron_trim);
        packet[0] = throttle;
        packet[1] = rudder;
        packet[3] = elevator;
        packet[4] = aileron;
  //      if(Model.protocol == PROTOCOL_YD717 && Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_YD717) {
//            packet[2] = elevator_trim;
//            packet[5] = aileron_trim;
//            packet[6] = rudder_trim;
//        } else {
            packet[2] = rudder_trim;
            packet[5] = elevator_trim;
            packet[6] = aileron_trim;
//        }
        packet[7] = flags;
    }


    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, (_BV(NRF24L01_07_TX_DS) | _BV(NRF24L01_07_MAX_RT)));
    NRF24L01_FlushTx();

//    if(Model.protocol == PROTOCOL_YD717 && Model.proto_opts[PROTOOPTS_FORMAT] == FORMAT_YD717) {
        NRF24L01_WritePayload(packet, 8);
//    } else {
//        packet[8] = packet[0];  // checksum
//        for(u8 i=1; i < 8; i++) packet[8] += packet[i];
//        packet[8] = ~packet[8];
//
//        NRF24L01_WritePayload(packet, 9);
//    }

    ++packet_counter;

//    radio.ce(HIGH);
//    delayMicroseconds(15);
    // It saves power to turn off radio after the transmission,
    // so as long as we have pins to do so, it is wise to turn
    // it back.
//    radio.ce(LOW);

    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != TXPOWER_150mW) {
        //Keep transmit power updated
        tx_power = TXPOWER_150mW;
        NRF24L01_SetPower(tx_power);
    }
}

static void yd717_init()
{
    NRF24L01_Initialize();

    // CRC, radio on
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, _BV(NRF24L01_00_EN_CRC) | _BV(NRF24L01_00_PWR_UP)); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x3F);      // Auto Acknoledgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x3F);  // Enable all data pipes
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x1A); // 500uS retransmit t/o, 10 tries
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_CHANNEL);      // Channel 3C
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
    NRF24L01_SetPower(TXPOWER_150mW);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_0C_RX_ADDR_P2, 0xC3); // LSB byte of pipe 2 receive address
    NRF24L01_WriteReg(NRF24L01_0D_RX_ADDR_P3, 0xC4);
    NRF24L01_WriteReg(NRF24L01_0E_RX_ADDR_P4, 0xC5);
    NRF24L01_WriteReg(NRF24L01_0F_RX_ADDR_P5, 0xC6);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PAYLOADSIZE);   // bytes of data payload for pipe 1
    NRF24L01_WriteReg(NRF24L01_12_RX_PW_P1, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_13_RX_PW_P2, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_14_RX_PW_P3, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_15_RX_PW_P4, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_16_RX_PW_P5, PAYLOADSIZE);
    NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);       // Enable dynamic payload length on all pipes

    // this sequence necessary for module from stock tx
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_Activate(0x73);                          // Activate feature register
    NRF24L01_ReadReg(NRF24L01_1D_FEATURE);
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x3F);       // Enable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x07);     // Set feature bits on


    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);

}


static void YD717_init1()
{
    // for bind packets set address to prearranged value known to receiver
    byte bind_rx_tx_addr[5];// = {0x65, 0x65, 0x65, 0x65, 0x65};
    for(byte i=0; i < 5; i++) bind_rx_tx_addr[i]  = 0x60; // SymaX

    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bind_rx_tx_addr, 5);
}


static void YD717_init2()
{
    // set rx/tx address for data phase
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
}


static void set_rx_tx_addr(uint32_t id)
{
    rx_tx_addr[0] = (id >> 24) & 0xFF;
    rx_tx_addr[1] = (id >> 16) & 0xFF;
    rx_tx_addr[2] = (id >>  8) & 0xFF;
    rx_tx_addr[3] = (id >>  0) & 0xFF;
    rx_tx_addr[4] = 0xC1; // always uses first data port
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void initialize_rx_tx_addr()
{
    uint32_t lfsr = 0xb2c54a2ful;
    set_rx_tx_addr(lfsr);
}

static void initialize()
{
    tmr1_stop();
    tx_power = TXPOWER_150mW;
    initialize_rx_tx_addr();
    NRF24L01_SetTxRxMode(TX_EN);
    packet_counter = 0;
    flags = 0;

    yd717_init();
    phase = YD717_INIT1;

    tmr1_set(INITIAL_WAIT);
    tmr1_start(micros());
}

static uint16_t yd717_callback()
{
    switch (phase) {
    case YD717_INIT1:
        send_packet(0);      // receiver doesn't re-enter bind mode if connection lost...check if already bound
        phase = YD717_BIND3;
        Serial.println("rebind");
//        MUSIC_Play(MUSIC_TELEMALARM1);
        break;

    case YD717_BIND2:
        if (counter == 0) {
            if (packet_ack() == PKT_PENDING)
                return PACKET_CHKTIME;             // packet send not yet complete
            YD717_init2();                         // change to data phase rx/tx address
            send_packet(0);
            phase = YD717_BIND3;
        } else {
            if (packet_ack() == PKT_PENDING)
                return PACKET_CHKTIME;             // packet send not yet complete
            send_packet(1);
            counter -= 1;
        }
        break;

    case YD717_BIND3:
        switch (packet_ack()) {
        case PKT_PENDING:
            return PACKET_CHKTIME;                 // packet send not yet complete
        case PKT_ACKED:
            phase = YD717_DATA;
            Serial.println("bind");
//            MUSIC_Play(MUSIC_DONE_BINDING);
            break;
        case PKT_TIMEOUT:
            YD717_init1();                         // change to bind rx/tx address
            counter = BIND_COUNT;
            phase = YD717_BIND2;
            send_packet(1);
        }
        break;

    case YD717_DATA:
        if (packet_ack() == PKT_PENDING)
            return PACKET_CHKTIME;                 // packet send not yet complete
        send_packet(0);
        break;
    }
    return PACKET_PERIOD;                          // Packet every 8ms
}

/* usage:
  Init: initalize();
  Deinit: NRF24L01_Reset();
  Reset: NRF24L01_Reset();
  bind: initalize();
  set_tx_power: NRF24L01_SetPower(tx_power);;
*/

void setup()
{
  Serial.begin(115200);
  
  // Setup SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setBitOrder(MSBFIRST);
  
  // Activate Chip Enable
  pinMode(CE,OUTPUT);
  pinMode(pinLED,OUTPUT);
  pinMode(CS,OUTPUT);
  digitalWrite(CS,1);
  digitalWrite(CE,1);

  tmr2_set(500);
  tmr2_start(millis());
  
  initialize();
}

void loop() 
{ 
  if (tmr1_chk(micros()))  tmr1_set(yd717_callback());
if (phase==YD717_DATA) { digitalWrite(pinLED, 1); } else 
if (tmr2_chk(millis())) { digitalWrite(pinLED, 1-digitalRead(pinLED)); }
  if (Serial.available()) {
    switch(Serial.read()) {
      case '0': initialize(); break;
      case '1': NRF24L01_Reset(); break;
      case 'a': throttle = 255; break;
      case 'z': throttle = 0;   break;
      case 'f': flags ^= FLAG_FLIP;   break;
      case 'l': flags ^= FLAG_LIGHT;   break;
      case 'c': digitalWrite(CE,0);
                delayMicroseconds(200);
                digitalWrite(CE,1);
                break;
      
      case '?':
        Serial.print("packet_count=");  Serial.println(packet_counter);
        Serial.print("phase=");  Serial.println(phase);
        Serial.print("bind counter=");  Serial.println(counter);
        Serial.print("flags=");  Serial.println(flags,HEX);
        Serial.print("24l0_STATUS=");  Serial.println(NRF24L01_ReadReg(NRF24L01_07_STATUS), HEX);
        Serial.print("24l0_CH=");  Serial.println(NRF24L01_ReadReg(NRF24L01_05_RF_CH), HEX);
        Serial.print("CE=");  Serial.println(digitalRead(CE));
        Serial.println("---eof---");
        break;
    }
  }
}

