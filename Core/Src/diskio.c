#include "main.h"
#include "diskio.h"
#include "ff.h"

extern SPI_HandleTypeDef hspi1;

/* Utilitza les macros generades per STM32CubeIDE a main.h */
#define SD_SELECT()   HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define SD_DESELECT() HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)

/* Funció requerida per FatFS per assignar data/hora als fitxers */
DWORD get_fattime(void) {
    return ((DWORD)(2026 - 1980) << 25) /* Any 2026 */
         | ((DWORD)1 << 21)             /* Mes 1 */
         | ((DWORD)1 << 16)             /* Dia 1 */
         | ((DWORD)0 << 11)             /* Hora 0 */
         | ((DWORD)0 << 5)              /* Minut 0 */
         | ((DWORD)0 >> 1);             /* Segon 0 */;
}

/* Transmissió i recepció d'un byte per SPI */
static uint8_t SPI_RxTx(uint8_t data) {
    uint8_t rx = 0xFF;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

/* Enviament de comandes a la SD */
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    uint8_t res, count = 0;

    // Trama de la comanda SPI (Index + Arguments + CRC)
    SPI_RxTx(cmd | 0x40);
    SPI_RxTx((uint8_t)(arg >> 24));
    SPI_RxTx((uint8_t)(arg >> 16));
    SPI_RxTx((uint8_t)(arg >> 8));
    SPI_RxTx((uint8_t)arg);
    SPI_RxTx(crc);

    // Espera la resposta (primer byte amb el bit 7 a 0)
    do {
        res = SPI_RxTx(0xFF);
        count++;
    } while ((res & 0x80) && count < 0xFF);

    return res;
}

DSTATUS disk_status (BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return 0;
}

DSTATUS disk_initialize (BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;

    // 1. Enviar 80+ polsos de clock amb CS desconnectat (ALT) per "despertar" la SD
    SD_DESELECT();
    for (int i = 0; i < 10; i++) {
        SPI_RxTx(0xFF);
    }

    // 2. CMD0: Inicialitzar en mode SPI
    SD_SELECT();
    if (SD_SendCmd(0, 0, 0x95) != 0x01) {
        SD_DESELECT();
        SPI_RxTx(0xFF);
        return STA_NOINIT;
    }

    // 3. CMD8: Verificar interfície SD v2 (obligatori per a MicroSDs SDHC/SDXC)
    if (SD_SendCmd(8, 0x000001AA, 0x87) == 0x01) {
        // Descartar els 4 bytes de resposta de la comanda CMD8
        for (int i = 0; i < 4; i++) {
            SPI_RxTx(0xFF);
        }
    }

    // 4. ACMD41: Inicialitzar el controlador de la SD
    uint16_t timeout = 1000;
    uint8_t res;
    do {
        SD_SendCmd(55, 0, 0x65);                 // CMD55 (Prefix per ACMD)
        res = SD_SendCmd(41, 0x40000000, 0x77); // ACMD41 amb HCS bit = 1
        HAL_Delay(1);
        timeout--;
    } while (res != 0x00 && timeout > 0);

    SD_DESELECT();
    SPI_RxTx(0xFF); // Byte extra per a finalitzar el bus SPI

    return (timeout > 0) ? 0 : STA_NOINIT;
}

DRESULT disk_read (BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || !count) return RES_PARERR;

    SD_SELECT();

    for (UINT i = 0; i < count; i++) {
        // CMD17: Llegir un bloc de dades
        if (SD_SendCmd(17, sector + i, 0xFF) != 0x00) {
            SD_DESELECT();
            SPI_RxTx(0xFF);
            return RES_ERROR;
        }

        // Esperar el token d'inici de dades (0xFE)
        uint16_t timeout = 5000;
        while (SPI_RxTx(0xFF) != 0xFE && timeout > 0) {
            timeout--;
        }

        if (timeout == 0) {
            SD_DESELECT();
            SPI_RxTx(0xFF);
            return RES_ERROR;
        }

        // Llegir el sector de 512 bytes
        for (int j = 0; j < 512; j++) {
            buff[j] = SPI_RxTx(0xFF);
        }

        // Descartar els 2 bytes de CRC
        SPI_RxTx(0xFF);
        SPI_RxTx(0xFF);

        buff += 512;
    }

    SD_DESELECT();
    SPI_RxTx(0xFF);
    return RES_OK;
}

DRESULT disk_write (BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || !count) return RES_PARERR;

    SD_SELECT();

    for (UINT i = 0; i < count; i++) {
        // CMD24: Escriure un bloc de dades
        if (SD_SendCmd(24, sector + i, 0xFF) != 0x00) {
            SD_DESELECT();
            SPI_RxTx(0xFF);
            return RES_ERROR;
        }

        // Enviar el token d'inici de dades (0xFE)
        SPI_RxTx(0xFE);

        // Enviar el bloc de 512 bytes
        for (int j = 0; j < 512; j++) {
            SPI_RxTx(buff[j]);
        }

        // Enviar bytes Dummy de CRC
        SPI_RxTx(0xFF);
        SPI_RxTx(0xFF);

        // Comprovar si la SD ha acceptat el bloc (Data Response Token)
        if ((SPI_RxTx(0xFF) & 0x1F) != 0x05) {
            SD_DESELECT();
            SPI_RxTx(0xFF);
            return RES_ERROR;
        }

        // Esperar que la SD acabi d'escriure internament (Bus Busy)
        while (SPI_RxTx(0xFF) == 0x00);

        buff += 512;
    }

    SD_DESELECT();
    SPI_RxTx(0xFF);
    return RES_OK;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != 0) return RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC:
            SD_SELECT();
            while (SPI_RxTx(0xFF) == 0x00);
            SD_DESELECT();
            SPI_RxTx(0xFF);
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;

        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(LBA_t*)buff = 2048000; // Valor per defecte (~1GB), suficient per operacions basiques
            return RES_OK;

        default:
            return RES_PARERR;
    }
}
