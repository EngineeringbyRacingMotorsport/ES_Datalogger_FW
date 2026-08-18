#include "microsd.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

// Control manual del Pin CS (Chip Select) a PA3
#define SD_CS_PORT    GPIOA
#define SD_CS_PIN     GPIO_PIN_3

#define SD_SELECT()   HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET) // Baix = Actiu
#define SD_DESELECT() HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET)   // Alt = Inactiu

// Variables de treball de FatFS
FATFS FatFs;         // Handle del sistema de fitxers
FIL myFile;          // Handle del fitxer actiu
volatile FRESULT fresult;     // Estat de les operacions de FatFS
UINT bytesWritten;   // Bytes escrits realment
extern SPI_HandleTypeDef hspi1;

// Flags d'estat i nom de fitxer actiu
static uint8_t sd_is_initialized = 0;
static char current_filename[32] = {0};

/**
 * @brief Cerca un nom de fitxer no existent incrementant l'índex (DataLogEM06_X.csv).
 */
static void Find_Next_Filename(void) {
    FILINFO fno;
    uint32_t file_index = 1;

    while (file_index < 10000) {
        // Genera noms com: /LOG_0001.csv (8 caràcters exactes de nom)
        snprintf(current_filename, sizeof(current_filename), "/LOG_%04lu.csv", (unsigned long)file_index);

        // Crida neta a FatFS sense SD_SELECT / SD_DESELECT
        fresult = f_stat(current_filename, &fno);

        if (fresult == FR_NO_FILE) {
            // El fitxer no existeix, és el primer nom disponible
            break;
        }
        file_index++;
    }
}

/**
 * @brief Inicialitza la targeta MicroSD, crea el nou fitxer CSV i hi escriu la capçalera.
 * @return 0 en cas d'èxit, -1 si hi ha error.
 */
int MicroSD_Init(void) {
    // 1. Assegurar que la SD està desseleccionada (CS = 1)
    SD_DESELECT();

    // 2. Enviar 10 bytes "dummy" (0xFF) amb CS en Alt per inicialitzar l'estat SPI de la SD
    uint8_t dummy = 0xFF;
    for (int i = 0; i < 10; i++) {
        HAL_SPI_Transmit(&hspi1, &dummy, 1, 10);
    }

    // 3. Muntar el sistema de fitxers FatFS
    // NO s'ha de fer SD_SELECT() / SD_DESELECT() aquí, f_mount ja ho gestiona internament.
    fresult = f_mount(&FatFs, "", 1);

    if (fresult != FR_OK) {
        sd_is_initialized = 0;
        return -1;
    }

    sd_is_initialized = 1;

    // 4. Cercar el següent nom disponible
    Find_Next_Filename();

    // 5. Crear el fitxer nou i escriure la capçalera
    // Tampoc es posa SD_SELECT() / SD_DESELECT() al voltant de f_open ni f_write
    fresult = f_open(&myFile, current_filename, FA_CREATE_ALWAYS | FA_WRITE);

    if (fresult == FR_OK) {
    	const char *header = "Timestamp_ms,"
    	                     "FpDIGRpot,FpDIGLpot,FpDIGRvel,FpDIGLvel,FpANLbrake,"
    	                     "FpINTr2d,FpSDCinertia,FpSDCbots,FpSDCcsdb,FpERRapps,"
    	                     "FpDIGr2d,FpINTrefrion,FpSHU,FpANLRpot,"
    	                     "RpSDChvd,RpSDCtsms,RpSDCrsdb,RpSDClsdb,RpSTAbrkledR,"
    	                     "RpSHU,"
    	                     "ApTHRhv,ApSHU,"
    	                     "BpTHRbrake,BpTHRcurrent,BpERRplaus,BpSDC,DpSDC,DpTHRhv,"
    	                     "BpSHU,DpSHU,"
    	                     "TpDIGspre,TpDIGsairp,TpDIGsairn,TpDIGipre,TpDIGiairp,TpDIGiairn,"
    	                     "TpTHRhv,TpERRscs,TpTHRdis,TpLCH,TpINTled,"
    	                     "SpERRbms,SpERRimd,SpLCHebms,SpLCHeimd,SpINTresbut,SpSDCbms,SpSDCimd,SpSHU,"
    	                     "IpANLmaxt,MpANLmaxt,BpANLbatc,BpANLmaxt,BpANLbatv\n";

        f_write(&myFile, header, strlen(header), &bytesWritten);
        f_sync(&myFile);
        f_close(&myFile);
    } else {
        sd_is_initialized = 0;
        return -1;
    }

    return 0;
}

/**
 * @brief Guarda l'estat actual de l'estructura DICCP a la MicroSD en format CSV.
 * @param DICCP Punter a l'estructura de dades del vehicle.
 * @return 0 si s'ha escrit correctament, -1 si hi ha error.
 */
int MicroSD_LogData(volatile DICCP_t *DICCP) {
    if (!sd_is_initialized) {
        if (MicroSD_Init() != 0) {
            return -1;
        }
    }

    // Buffer de 768 bytes per prevenir truncaments
    char buffer[768];
    uint32_t timestamp = HAL_GetTick();

    int len = snprintf(buffer, sizeof(buffer),
            "%lu,"
            "%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,"
            "%u,"
            "%u,%u,"
            "%u,%u,%u,%u,%u,%u,"
            "%u,%u,"
            "%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u,%u\n",
        (unsigned long)timestamp,
        // FrontECU
        DICCP->FpDIGRpot,
		DICCP->FpDIGLpot,
		DICCP->FpDIGRvel,
		DICCP->FpDIGLvel,
		DICCP->FpANLbrake,
		DICCP->FpINTr2d,
        DICCP->FpSDCinertia,
		DICCP->FpSDCbots,
		DICCP->FpSDCcsdb,
		DICCP->FpERRapps,
		DICCP->FpDIGr2d,
		DICCP->FpINTrefrion,
        DICCP->FpSHU,
		DICCP->FpANLRpot,
        // RearECU
        DICCP->RpSDChvd,
		DICCP->RpSDCtsms,
		DICCP->RpSDCrsdb,
		DICCP->RpSDClsdb,
		DICCP->RpSTAbrkledR,
        DICCP->RpSHU,
        // HVAB
        DICCP->ApTHRhv,
		DICCP->ApSHU,
        // HVDB
        DICCP->BpTHRbrake,
		DICCP->BpTHRcurrent,
		DICCP->BpERRplaus,
		DICCP->BpSDC,
		DICCP->DpSDC,
		DICCP->DpTHRhv,
        DICCP->BpSHU,
		DICCP->DpSHU,
        // TSALGreen
        DICCP->TpDIGspre,
		DICCP->TpDIGsairp,
		DICCP->TpDIGsairn,
		DICCP->TpDIGipre,
		DICCP->TpDIGiairp,
		DICCP->TpDIGiairn,
		DICCP->TpTHRhv,
		DICCP->TpERRscs,
		DICCP->TpTHRdis,
		DICCP->TpLCH,
		DICCP->TpINTled,
        // SDCReset
        DICCP->SpERRbms,
		DICCP->SpERRimd,
		DICCP->SpLCHebms,
		DICCP->SpLCHeimd,
		DICCP->SpINTresbut,
		DICCP->SpSDCbms,
		DICCP->SpSDCimd,
		DICCP->SpSHU,
		//Inverter
		DICCP->IpANLmaxt,
		//Motor
		DICCP->MpANLmaxt,
		//BMS
		DICCP->BpANLbatc,
		DICCP->BpANLmaxt,
		DICCP->BpANLbatv
    );

    if (len <= 0) return -1;

    // Obrir fitxer en mode apèndix, escriure i tancar
    SD_SELECT();
    fresult = f_open(&myFile, current_filename, FA_OPEN_APPEND | FA_WRITE);

    if (fresult != FR_OK) {
        SD_DESELECT();
        return -1;
    }

    fresult = f_write(&myFile, buffer, len, &bytesWritten);
    f_sync(&myFile);
    f_close(&myFile);
    SD_DESELECT();

    if (fresult != FR_OK || bytesWritten < (UINT)len) {
        return -1;
    }

    return 0;
}

/**
 * @brief Desmunta el sistema de fitxers de la MicroSD de forma segura.
 */
void MicroSD_Unmount(void) {
    SD_SELECT();
    f_mount(NULL, "", 1);
    SD_DESELECT();
    sd_is_initialized = 0;
}
