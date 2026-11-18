#include "microsd.h"
#include "cpf.h"
#include "ff.h"
#include <stdio.h>

/*
 * Cada trama de CAN rebuda es guarda a la MicroSD com un paquet CPF, el format
 * que llegeix l'ES_CANEMviewer_SW. La interrupció de CAN codifica la trama al
 * buffer circular (MicroSD_OnFrame) i el bucle principal l'escriu al fitxer
 * (MicroSD_Task), així la interrupció mai espera la SD.
 */

#define SD_BLOCK_SIZE       512U
#define SD_SYNC_PERIOD_MS   1000U  /* Màxim de dades perdudes si es talla l'alimentació */
#define SD_RETRY_PERIOD_MS  1000U  /* Cada quant es torna a provar de muntar la SD */

/*
 * La SD s'ha d'inicialitzar a 400 kHz o menys (48 MHz / 128 = 375 kHz). Un cop
 * muntada, es passa a 48 MHz / 4 = 12 MHz.
 */
#define SD_SPI_PRESCALER_INIT  SPI_BAUDRATEPRESCALER_128
#define SD_SPI_PRESCALER_FAST  SPI_BAUDRATEPRESCALER_4

// Variables de treball de FatFS
FATFS FatFs;         // Handle del sistema de fitxers
FIL myFile;          // Handle del fitxer actiu
volatile FRESULT fresult;     // Estat de les operacions de FatFS
extern SPI_HandleTypeDef hspi1;

/* Trames que no s'han pogut guardar perquè el buffer estava ple */
volatile uint32_t MicroSD_DroppedFrames = 0;

// Flags d'estat i nom de fitxer actiu
static uint8_t sd_is_initialized = 0;
static char current_filename[16] = {0};

/* La interrupció només guarda trames mentre hi ha un fitxer obert */
static volatile uint8_t recording = 0;
/* Tick en què es va crear el fitxer: els timestamps hi són relatius */
static volatile uint32_t record_start_tick = 0;

static uint32_t last_sync_tick = 0;
static uint32_t last_retry_tick = 0;

static void SPI_SetPrescaler(uint32_t prescaler) {
    if (hspi1.Init.BaudRatePrescaler == prescaler) {
        return;
    }
    hspi1.Init.BaudRatePrescaler = prescaler;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief Cerca el primer nom de fitxer que no existeixi (/LOG_0001.cpf, ...).
 * @return 0 si n'ha trobat un, -1 si no.
 */
static int Find_Next_Filename(void) {
    FILINFO fno;

    for (uint32_t file_index = 1; file_index < 10000; file_index++) {
        // Noms 8.3, ja que FatFS no té noms llargs (FF_USE_LFN = 0)
        snprintf(current_filename, sizeof(current_filename), "/LOG_%04lu.cpf", (unsigned long)file_index);

        fresult = f_stat(current_filename, &fno);
        if (fresult == FR_NO_FILE) {
            return 0;
        }
        if (fresult != FR_OK) {
            return -1;
        }
    }
    return -1;
}

/**
 * @brief Escriu al fitxer fins a n bytes pendents del buffer circular.
 * @return 0 en cas d'èxit, -1 si hi ha error.
 */
static int Write_Pending(uint32_t n) {
    while (n > 0U) {
        const uint8_t *chunk;
        uint32_t len = CPF_Ring_Peek(&chunk);
        if (len == 0U) {
            break;
        }
        if (len > n) {
            len = n;
        }

        UINT written = 0;
        fresult = f_write(&myFile, chunk, len, &written);
        if (fresult != FR_OK || written != len) {
            return -1;
        }
        CPF_Ring_Consume(len);
        n -= len;
    }
    return 0;
}

/**
 * @brief Atura la gravació i allibera la SD després d'un error.
 */
static void Fail(void) {
    recording = 0;
    f_close(&myFile);
    f_mount(NULL, "", 0);
    sd_is_initialized = 0;
    last_retry_tick = HAL_GetTick();
}

/**
 * @brief Inicialitza la targeta MicroSD i crea un fitxer CPF nou.
 * @return 0 en cas d'èxit, -1 si hi ha error.
 */
int MicroSD_Init(void) {
    recording = 0;
    sd_is_initialized = 0;

    // 1. Velocitat d'inicialització de la SD
    SPI_SetPrescaler(SD_SPI_PRESCALER_INIT);

    // 2. Enviar 10 bytes "dummy" (0xFF) amb CS en Alt per inicialitzar l'estat SPI de la SD
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
    uint8_t dummy = 0xFF;
    for (int i = 0; i < 10; i++) {
        HAL_SPI_Transmit(&hspi1, &dummy, 1, 10);
    }

    // 3. Muntar el sistema de fitxers FatFS (f_mount ja gestiona el CS)
    fresult = f_mount(&FatFs, "", 1);
    if (fresult != FR_OK) {
        return -1;
    }

    // 4. Un cop muntada, la SD accepta un rellotge més ràpid
    SPI_SetPrescaler(SD_SPI_PRESCALER_FAST);

    // 5. Crear el fitxer nou. El format CPF no té capçalera.
    if (Find_Next_Filename() != 0) {
        f_mount(NULL, "", 0);
        return -1;
    }

    fresult = f_open(&myFile, current_filename, FA_CREATE_NEW | FA_WRITE);
    if (fresult != FR_OK) {
        f_mount(NULL, "", 0);
        return -1;
    }

    sd_is_initialized = 1;

    // 6. Començar a gravar amb el buffer buit i els timestamps a 0
    CPF_Ring_Reset();
    record_start_tick = HAL_GetTick();
    last_sync_tick = record_start_tick;
    recording = 1;

    return 0;
}

/**
 * @brief Guarda una trama de CAN rebuda. Es crida des de la interrupció de CAN.
 */
void MicroSD_OnFrame(uint32_t id, const uint8_t *data, uint32_t len) {
    if (!recording) {
        return;
    }

    uint32_t timestamp = HAL_GetTick() - record_start_tick;
    if (!CPF_Ring_Push(id, timestamp, data, len)) {
        MicroSD_DroppedFrames++;
    }
}

/**
 * @brief Escriu a la MicroSD les trames rebudes. S'ha de cridar al bucle principal.
 */
void MicroSD_Task(void) {
    uint32_t now = HAL_GetTick();

    if (!sd_is_initialized) {
        if ((now - last_retry_tick) >= SD_RETRY_PERIOD_MS) {
            last_retry_tick = now;
            MicroSD_Init();
        }
        return;
    }

    // Escriure els blocs sencers que hi hagi ara, sense esperar les trames que arribin mentrestant
    uint32_t full_blocks = CPF_Ring_Available() & ~(SD_BLOCK_SIZE - 1U);
    if (Write_Pending(full_blocks) != 0) {
        Fail();
        return;
    }

    // Periòdicament, escriure també la resta i actualitzar el fitxer a la SD
    if ((now - last_sync_tick) >= SD_SYNC_PERIOD_MS) {
        last_sync_tick = now;
        if (Write_Pending(CPF_Ring_Available()) != 0) {
            Fail();
            return;
        }
        fresult = f_sync(&myFile);
        if (fresult != FR_OK) {
            Fail();
        }
    }
}

/**
 * @brief Escriu les trames pendents, tanca el fitxer i desmunta la MicroSD.
 */
void MicroSD_Unmount(void) {
    if (sd_is_initialized) {
        recording = 0;
        Write_Pending(CPF_Ring_Available());
        f_close(&myFile);
    }
    f_mount(NULL, "", 0);
    sd_is_initialized = 0;
}
