#ifndef _IFACE_NRF24L01_H_
#define _IFACE_NRF24L01_H_

enum pins { CE =  9, CS = 10 };

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

enum TXRX_State {
    TXRX_OFF,
    TX_EN,
    RX_EN,
};

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
//#define NOP           0xFF

void NRF24L01_Initialize();
 int8_t NRF24L01_Reset();
uint8_t NRF24L01_WriteReg(uint8_t reg, uint8_t data);
uint8_t NRF24L01_WriteRegisterMulti(uint8_t reg, const uint8_t data[], uint8_t length);
uint8_t NRF24L01_WritePayload(uint8_t *data, uint8_t len);
uint8_t NRF24L01_ReadReg(uint8_t reg);
uint8_t NRF24L01_ReadRegisterMulti(uint8_t reg, uint8_t data[], uint8_t length);
uint8_t NRF24L01_ReadPayload(uint8_t *data, uint8_t len);

uint8_t NRF24L01_FlushTx();
uint8_t NRF24L01_FlushRx();
uint8_t NRF24L01_Activate(uint8_t code);


// Bitrate 0 - 1Mbps, 1 - 2Mbps, 3 - 250K (for nRF24L01+)
uint8_t NRF24L01_SetBitrate(uint8_t bitrate);

uint8_t NRF24L01_SetPower(uint8_t power);
void NRF24L01_SetTxRxMode(enum TXRX_State);

// To enable radio transmit after WritePayload you need to turn the radio
//void NRF24L01_PulseCE();

#endif
