/**
 * @file  rc522.c
 * @brief RC522 RFID driver — SPI polling, no external library.
 *        Pin assignments are in rc522.h
 */

#include "rc522.h"

/* ── Authorised UID table ──────────────────────────────────────────────
 * Starts empty — UIDs are enrolled at runtime via Enroll_Mode()
 * in main.c. Once you know your real UIDs you can hard-code them:
 *
 *   static uint8_t authorised_uids[RC522_MAX_UIDS][RC522_UID_LEN] = {
 *       {0xA3, 0xF2, 0x01, 0x9C},
 *   };
 *   static uint8_t uid_count = 1;
 * ──────────────────────────────────────────────────────────────────── */
static uint8_t authorised_uids[RC522_MAX_UIDS][RC522_UID_LEN];
static uint8_t uid_count = 0;

/* ── CS / RST helpers ───────────────────────────────────────────────── */
static inline void CS_Low(void)  { HAL_GPIO_WritePin(RFID_NSS_PORT, RFID_NSS_PIN, GPIO_PIN_RESET); }
static inline void CS_High(void) { HAL_GPIO_WritePin(RFID_NSS_PORT, RFID_NSS_PIN, GPIO_PIN_SET);   }

/* ── Register I/O ───────────────────────────────────────────────────── */
static void WriteReg(uint8_t reg, uint8_t val) {
    uint8_t tx[2] = { (reg << 1) & 0x7E, val };
    CS_Low();
    HAL_SPI_Transmit(&RFID_SPI_HANDLE, tx, 2, 10);
    CS_High();
}

static uint8_t ReadReg(uint8_t reg) {
    uint8_t tx = ((reg << 1) & 0x7E) | 0x80;
    uint8_t rx = 0;
    CS_Low();
    HAL_SPI_Transmit(&RFID_SPI_HANDLE, &tx, 1, 10);
    HAL_SPI_Receive(&RFID_SPI_HANDLE,  &rx, 1, 10);
    CS_High();
    return rx;
}

static void SetBits(uint8_t reg, uint8_t mask) { WriteReg(reg, ReadReg(reg) |  mask); }
static void ClrBits(uint8_t reg, uint8_t mask) { WriteReg(reg, ReadReg(reg) & ~mask); }

/* ── Init ────────────────────────────────────────────────────────────── */
void RC522_Init(void) {
    /* Hard reset via RST pin */
    HAL_GPIO_WritePin(RFID_RST_PORT, RFID_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RFID_RST_PORT, RFID_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);

    /* Soft reset */
    WriteReg(MFRC522_REG_COMMAND, PCD_RESETPHASE);
    HAL_Delay(10);

    /* Timer config: ~20 ms timeout */
    WriteReg(MFRC522_REG_T_MODE,      0x8D);
    WriteReg(MFRC522_REG_T_PRESCALER, 0x3E);
    WriteReg(MFRC522_REG_T_RELOAD_L,  0x1E);
    WriteReg(MFRC522_REG_T_RELOAD_H,  0x00);

    WriteReg(MFRC522_REG_TX_AUTO, 0x40);   /* 100% ASK modulation */
    WriteReg(MFRC522_REG_MODE,    0x3D);   /* CRC 0x6363 */

    /* Antenna ON */
    if (!(ReadReg(MFRC522_REG_TX_CONTROL) & 0x03))
        SetBits(MFRC522_REG_TX_CONTROL, 0x03);
}

/* ── Core transceive ─────────────────────────────────────────────────── */
static uint8_t ToCard(uint8_t cmd,
                       uint8_t *sendData, uint8_t sendLen,
                       uint8_t *backData, uint16_t *backLen) {
    uint8_t  status  = MI_ERR;
    uint8_t  irqEn   = (cmd == PCD_TRANSCEIVE) ? 0x77 : 0x00;
    uint8_t  waitIRq = (cmd == PCD_TRANSCEIVE) ? 0x30 : 0x00;
    uint8_t  n;
    uint32_t i;

    WriteReg(MFRC522_REG_COM_IEN,   irqEn | 0x80);
    ClrBits(MFRC522_REG_COM_IRQ,    0x80);
    SetBits(MFRC522_REG_FIFO_LEVEL, 0x80);
    WriteReg(MFRC522_REG_COMMAND,   PCD_IDLE);

    for (i = 0; i < sendLen; i++)
        WriteReg(MFRC522_REG_FIFO_DATA, sendData[i]);

    WriteReg(MFRC522_REG_COMMAND, cmd);
    if (cmd == PCD_TRANSCEIVE)
        SetBits(MFRC522_REG_BIT_FRAMING, 0x80);

    i = 2000;
    do { n = ReadReg(MFRC522_REG_COM_IRQ); i--; }
    while (i && !(n & 0x01) && !(n & waitIRq));

    ClrBits(MFRC522_REG_BIT_FRAMING, 0x80);

    if (!i) return MI_ERR;

    if (!(ReadReg(MFRC522_REG_ERROR) & 0x1B)) {
        status = MI_OK;
        if (n & irqEn & 0x01) status = MI_NOTAGERR;
        if (cmd == PCD_TRANSCEIVE) {
            uint8_t rx   = ReadReg(MFRC522_REG_FIFO_LEVEL);
            uint8_t last = ReadReg(MFRC522_REG_CONTROL) & 0x07;
            *backLen = last ? (rx - 1) * 8 + last : rx * 8;
            if (rx == 0) rx = 1;
            if (rx > 16) rx = 16;
            for (i = 0; i < rx; i++)
                backData[i] = ReadReg(MFRC522_REG_FIFO_DATA);
        }
    }
    return status;
}

/* ── Public API ─────────────────────────────────────────────────────── */
uint8_t RC522_Request(uint8_t reqMode, uint8_t *tagType) {
    uint16_t backBits;
    WriteReg(MFRC522_REG_BIT_FRAMING, 0x07);
    tagType[0] = reqMode;
    uint8_t status = ToCard(PCD_TRANSCEIVE, tagType, 1, tagType, &backBits);
    return ((status == MI_OK) && (backBits == 0x10)) ? MI_OK : MI_ERR;
}

uint8_t RC522_Anticoll(uint8_t *uid) {
    uint16_t unLen;
    uint8_t  tx[2] = { PICC_ANTICOLL, 0x20 };
    WriteReg(MFRC522_REG_BIT_FRAMING, 0x00);
    uint8_t status = ToCard(PCD_TRANSCEIVE, tx, 2, uid, &unLen);
    if (status == MI_OK) {
        uint8_t chk = 0;
        for (uint8_t i = 0; i < 4; i++) chk ^= uid[i];
        if (chk != uid[4]) status = MI_ERR;
    }
    return status;
}

uint8_t RC522_IsAuthorised(uint8_t *uid) {
    for (uint8_t i = 0; i < uid_count; i++)
        if (memcmp(uid, authorised_uids[i], RC522_UID_LEN) == 0) return 1;
    return 0;
}

void RC522_AddUID(uint8_t *uid) {
    if (uid_count >= RC522_MAX_UIDS) return;
    memcpy(authorised_uids[uid_count++], uid, RC522_UID_LEN);
}
