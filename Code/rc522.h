#ifndef RC522_H
#define RC522_H

#include "stm32f4xx_hal.h"
#include <string.h>

/* ── Wiring (matching your actual hardware) ────────────────────────────
   RC522 SCK  → PA5      RC522 MISO → PA6
   RC522 MOSI → PA7      RC522 SDA  → PA4  (software CS)
   RC522 RST  → PB0      RC522 VCC  → 3.3 V  (NEVER 5 V)
   ──────────────────────────────────────────────────────────────────── */

#define RFID_SPI_HANDLE      hspi1

#define RFID_NSS_PORT        GPIOA          /* CS  = PA4 */
#define RFID_NSS_PIN         GPIO_PIN_4

#define RFID_RST_PORT        GPIOB          /* RST = PB0 */
#define RFID_RST_PIN         GPIO_PIN_0

/* RC522 registers */
#define MFRC522_REG_COMMAND      0x01
#define MFRC522_REG_COM_IEN      0x02
#define MFRC522_REG_COM_IRQ      0x04
#define MFRC522_REG_ERROR        0x06
#define MFRC522_REG_FIFO_DATA    0x09
#define MFRC522_REG_FIFO_LEVEL   0x0A
#define MFRC522_REG_CONTROL      0x0C
#define MFRC522_REG_BIT_FRAMING  0x0D
#define MFRC522_REG_MODE         0x11
#define MFRC522_REG_TX_CONTROL   0x14
#define MFRC522_REG_TX_AUTO      0x15
#define MFRC522_REG_T_MODE       0x2A
#define MFRC522_REG_T_PRESCALER  0x2B
#define MFRC522_REG_T_RELOAD_H   0x2C
#define MFRC522_REG_T_RELOAD_L   0x2D

/* PCD commands */
#define PCD_IDLE         0x00
#define PCD_TRANSCEIVE   0x0C
#define PCD_RESETPHASE   0x0F

/* PICC commands */
#define PICC_REQIDL      0x26
#define PICC_ANTICOLL    0x93

/* Status codes */
#define MI_OK            0
#define MI_NOTAGERR      1
#define MI_ERR           2

/* UID table config */
#define RC522_UID_LEN    4
#define RC522_MAX_UIDS   10

extern SPI_HandleTypeDef hspi1;

/* Public API */
void    RC522_Init(void);
uint8_t RC522_Request(uint8_t reqMode, uint8_t *tagType);
uint8_t RC522_Anticoll(uint8_t *uid);
uint8_t RC522_IsAuthorised(uint8_t *uid);
void    RC522_AddUID(uint8_t *uid);

#endif /* RC522_H */
