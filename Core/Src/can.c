#include <can.h>

#define PARSE_PERIOD_MS 200

extern volatile DICCP_t DICCP;

// Variables estàtiques per guardar l'últim timestamp de processament (en ms)
static uint32_t last_tick_FrontECU      = 0;
static uint32_t last_tick_RearECU       = 0;
static uint32_t last_tick_HVAB          = 0;
static uint32_t last_tick_HVDB          = 0;
static uint32_t last_tick_TSALG         = 0;
static uint32_t last_tick_SDCReset      = 0;
static uint32_t last_tick_Inverter_103  = 0;
static uint32_t last_tick_BMS_540       = 0;
static uint32_t last_tick_BMS_560       = 0;
static uint32_t last_tick_BMS_580       = 0;

// ==========================================
// Funcions de processament per placa
// ==========================================

static void Parse_FrontECU(volatile DICCP_t *DICCP, uint32_t can_id, uint8_t *data) {
	switch (can_id) {
	case 0x100:
		DICCP->FpDIGRpot = data[0];
		DICCP->FpDIGLpot = data[1];
		DICCP->FpDIGRvel = data[2];
		DICCP->FpDIGLvel = data[3];
		DICCP->FpANLbrake = data[4];
		break;

	case 0x101:
		DICCP->FpINTtsoff  = (data[0] >> 0) & 0x01;
		DICCP->FpINTsbms   = (data[0] >> 1) & 0x01;
		DICCP->FpINTr2d    = (data[0] >> 2) & 0x01;
		DICCP->FpINTmenu   = (data[0] >> 3) & 0x01;
		DICCP->FpDIGmicrosd= (data[0] >> 4) & 0x01;

		DICCP->FpSDCinertia= (data[1] >> 0) & 0x01;
		DICCP->FpSDCbots   = (data[1] >> 1) & 0x01;
		DICCP->FpSDCcsdb   = (data[1] >> 2) & 0x01;
		DICCP->FpERRapps   = (data[1] >> 3) & 0x01;
		DICCP->FpDIGrefri  = (data[1] >> 4) & 0x01;
		DICCP->FpDIGr2d    = (data[1] >> 5) & 0x01;
		DICCP->FpINTrefrion= (data[1] >> 6) & 0x01;

		DICCP->FpDIGvel    = data[2];

		DICCP->FpSHU       = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
		break;

	case 0x102:
		DICCP->FpANLRpot   = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
		break;

	default:
		break;
	}
}

static void Parse_RearECU(volatile DICCP_t *DICCP, uint32_t can_id, uint8_t *data) {
	switch (can_id) {
	case 0x200:
		DICCP->RpSDChvd     = (data[0] >> 0) & 0x01;
		DICCP->RpSDCtsms    = (data[0] >> 1) & 0x01;
		DICCP->RpSDCrsdb    = (data[0] >> 2) & 0x01;
		DICCP->RpSDClsdb    = (data[0] >> 3) & 0x01;
		DICCP->RpSTAbrkledR = (data[0] >> 4) & 0x01;
		DICCP->RpSTAbrkledG = (data[0] >> 5) & 0x01;
		DICCP->RpSTAbrkledB = (data[0] >> 6) & 0x01;
		DICCP->RpSIGlvs     = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
		DICCP->RpSHU        = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
		break;

	default:
		break;
	}
}

static void Parse_HVAB(volatile DICCP_t *DICCP, uint8_t *data) {
	DICCP->ApTHRhv = (data[0] >> 0) & 0x01;
	DICCP->ApSHU   = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
}

static void Parse_HVDB(volatile DICCP_t *DICCP, uint8_t *data) {
	DICCP->BpTHRbrake     = (data[0] >> 0) & 0x01;
	DICCP->BpTHRcurrent   = (data[0] >> 1) & 0x01;
	DICCP->BpERRplaus     = (data[0] >> 2) & 0x01;
	DICCP->DpINTtsalred   = (data[0] >> 3) & 0x01;
	DICCP->BpSDC          = (data[0] >> 4) & 0x01;
	DICCP->DpSDC          = (data[1] >> 0) & 0x01;
	DICCP->DpTHRhv        = (data[1] >> 1) & 0x01;
	DICCP->DpLCHdischarge = (data[1] >> 2) & 0x01;
	DICCP->DpSDCintlck1   = (data[1] >> 3) & 0x01;
	DICCP->DpSDCintlck2   = (data[1] >> 4) & 0x01;
	DICCP->BpSHU          = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
	DICCP->DpSHU          = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
}

static void Parse_TSALG(volatile DICCP_t *DICCP, uint8_t *data) {
	DICCP->TpDIGspre  = (data[0] >> 0) & 0x01;
	DICCP->TpDIGsairp = (data[0] >> 1) & 0x01;
	DICCP->TpDIGsairn = (data[0] >> 2) & 0x01;
	DICCP->TpDIGipre  = (data[0] >> 3) & 0x01;
	DICCP->TpDIGiairp = (data[0] >> 4) & 0x01;
	DICCP->TpDIGiairn = (data[0] >> 5) & 0x01;
	DICCP->TpTHRhv    = (data[1] >> 0) & 0x01;
	DICCP->TpERRscs   = (data[2] >> 1) & 0x01;
	DICCP->TpTHRdis   = (data[3] >> 2) & 0x01;
	DICCP->TpLCH      = (data[4] >> 3) & 0x01;
	DICCP->TpINTled   = (data[5] >> 4) & 0x01;
}

static void Parse_SDCReset(volatile DICCP_t *DICCP, uint8_t *data) {
	DICCP->SpERRbms    = (data[0] >> 0) & 0x01;
	DICCP->SpERRimd    = (data[0] >> 1) & 0x01;
	DICCP->SpLCHebms   = (data[0] >> 2) & 0x01;
	DICCP->SpLCHeimd   = (data[0] >> 3) & 0x01;
	DICCP->SpINTresbut = (data[0] >> 4) & 0x01;
	DICCP->SpSDCbms    = (data[0] >> 5) & 0x01;
	DICCP->SpSDCimd    = (data[0] >> 6) & 0x01;
	DICCP->SpSHU       = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
}

// ==========================================
// Inverter Parsers (CAN ID: 0x103)
// ==========================================

static void Parse_Inverter_0x103(volatile DICCP_t *DICCP, uint8_t *data) {
	uint8_t reg = data[0] & 0xFF;

	if (reg == 0x4A) {
		// Temperatura de l'Inversor (IGBT)
		uint16_t raw = (uint16_t)data[1] | ((uint16_t)data[2] << 8);

		static const uint16_t adc_lut[] = {
				16245, 16308, 16387, 16487, 16609, 16759, 16938, 17151, 17400, 17688,
				18017, 18387, 18797, 19247, 19733, 20250, 20793, 21357, 21933, 22515,
				23097, 23671, 24232, 24775, 25296, 25792, 26261, 26702, 27114, 27497,
				27851, 28179, 28480, 28757, 29011, 29243, 29456, 29650, 29827
		};

		static const int16_t temp_lut[] = {
				-35, -30, -25, -20, -15, -10,  -5,   0,   5,  10,
				15,  20,  25,  30,  35,  40,  45,  50,  55,  60,
				65,  70,  75,  80,  85,  90,  95, 100, 105, 110,
				115, 120, 125, 130, 135, 140, 145, 150, 155
		};

		uint8_t points = sizeof(adc_lut) / sizeof(adc_lut[0]);
		int16_t temp = temp_lut[0];

		if (raw <= adc_lut[0]) {
			temp = temp_lut[0];
		} else if (raw >= adc_lut[points - 1]) {
			temp = temp_lut[points - 1];
		} else {
			for (uint8_t i = 0; i < points - 1; i++) {
				if (raw >= adc_lut[i] && raw <= adc_lut[i + 1]) {
					int32_t x0 = adc_lut[i];
					int32_t x1 = adc_lut[i + 1];
					int32_t y0 = temp_lut[i];
					int32_t y1 = temp_lut[i + 1];

					temp = (int16_t)(y0 + ((int32_t)(raw - x0) * (y1 - y0)) / (x1 - x0));
					break;
				}
			}
		}

		DICCP->IpANLmaxt = temp;
	}
	else if (reg == 0x49) {
		// Temperatura del Motor
		uint16_t raw = (uint16_t)data[1] | ((uint16_t)data[2] << 8);

		static const uint16_t raw_lut[] = {
				7414,  7687,  7962,  8240,  8520,  8802,  9085,  9369,  9654,  9939,
				10225, 10510, 10795, 11080, 11364, 11646, 11927, 12207, 12485, 12762,
				13036, 13308, 13578, 13846, 14111, 14373, 14633, 14890, 15144, 15391,
				15628, 15852, 16061, 16251, 16421, 16569, 16692, 16789, 16857
		};

		static const int16_t temp_lut[] = {
				-35, -30, -25, -20, -15, -10,  -5,   0,   5,  10,
				15,  20,  25,  30,  35,  40,  45,  50,  55,  60,
				65,  70,  75,  80,  85,  90,  95, 100, 105, 110,
				115, 120, 125, 130, 135, 140, 145, 150, 155
		};

		const uint8_t lut_size = sizeof(raw_lut) / sizeof(raw_lut[0]);
		int32_t temp;

		if (raw <= raw_lut[0]) {
			temp = temp_lut[0];
		} else if (raw >= raw_lut[lut_size - 1]) {
			temp = temp_lut[lut_size - 1];
		} else {
			uint8_t i = 0;
			while (i < (lut_size - 1) && raw > raw_lut[i + 1]) {
				i++;
			}

			int32_t raw_min = raw_lut[i];
			int32_t raw_max = raw_lut[i + 1];
			int32_t t_min   = temp_lut[i];
			int32_t t_max   = temp_lut[i + 1];

			temp = t_min + (((int32_t)(raw - raw_min) * (t_max - t_min)) / (raw_max - raw_min));
		}

		DICCP->MpANLmaxt = (int16_t)temp;
	}
}

// ==========================================
// BMS Parsers
// ==========================================

static void Parse_BMS_0x540(volatile DICCP_t *DICCP, uint8_t *data) {
	// Corrent de la bateria
	uint16_t raw_current = ((uint16_t)data[0] << 8) | (uint16_t)data[1];
	int32_t val = (raw_current / 10) - 3200;

	DICCP->BpANLbatc = (val < 0) ? -val : val;
}

static void Parse_BMS_0x560(volatile DICCP_t *DICCP, uint8_t *data) {
	// Tensió de la bateria
	uint16_t raw_voltage = ((uint16_t)data[0] << 8) | (uint16_t)data[1];

	DICCP->BpANLbatv = raw_voltage / 10;
}

static void Parse_BMS_0x580(volatile DICCP_t *DICCP, uint8_t *data) {
	// Temperatura màxima de la bateria
	if ((data[0] & 0xFF) == 1) {
		DICCP->BpANLmaxt = ((data[1] & 0xFF) - 50);
	}
}

// ==========================================
// Encaminador principal de missatges
// ==========================================

static void StoreDataCAN(volatile DICCP_t *DICCP, uint32_t can_id, uint8_t *data) {
	uint32_t current_tick = HAL_GetTick();

	if (can_id >= 0x100 && can_id <= 0x102) {
		if ((current_tick - last_tick_FrontECU) >= PARSE_PERIOD_MS) {
			last_tick_FrontECU = current_tick;
			Parse_FrontECU(DICCP, can_id, data);
		}
	}
	else if (can_id >= 0x200 && can_id <= 0x202) {
		if ((current_tick - last_tick_RearECU) >= PARSE_PERIOD_MS) {
			last_tick_RearECU = current_tick;
			Parse_RearECU(DICCP, can_id, data);
		}
	}
	else if (can_id == 0x300) {
		if ((current_tick - last_tick_HVAB) >= PARSE_PERIOD_MS) {
			last_tick_HVAB = current_tick;
			Parse_HVAB(DICCP, data);
		}
	}
	else if (can_id == 0x400) {
		if ((current_tick - last_tick_HVDB) >= PARSE_PERIOD_MS) {
			last_tick_HVDB = current_tick;
			Parse_HVDB(DICCP, data);
		}
	}
	else if (can_id == 0x500) {
		if ((current_tick - last_tick_TSALG) >= PARSE_PERIOD_MS) {
			last_tick_TSALG = current_tick;
			Parse_TSALG(DICCP, data);
		}
	}
	else if (can_id == 0x600) {
		if ((current_tick - last_tick_SDCReset) >= PARSE_PERIOD_MS) {
			last_tick_SDCReset = current_tick;
			Parse_SDCReset(DICCP, data);
		}
	}
	else if (can_id == 0x103) {
		if ((current_tick - last_tick_Inverter_103) >= PARSE_PERIOD_MS) {
			last_tick_Inverter_103 = current_tick;
			Parse_Inverter_0x103(DICCP, data);
		}
	}
	else if (can_id == 0x540) {
		if ((current_tick - last_tick_BMS_540) >= PARSE_PERIOD_MS) {
			last_tick_BMS_540 = current_tick;
			Parse_BMS_0x540(DICCP, data);
		}
	}
	else if (can_id == 0x560) {
		if ((current_tick - last_tick_BMS_560) >= PARSE_PERIOD_MS) {
			last_tick_BMS_560 = current_tick;
			Parse_BMS_0x560(DICCP, data);
		}
	}
	else if (can_id == 0x580) {
		if ((current_tick - last_tick_BMS_580) >= PARSE_PERIOD_MS) {
			last_tick_BMS_580 = current_tick;
			Parse_BMS_0x580(DICCP, data);
		}
	}
}

// ==========================================
// Configuració i callbacks del perifèric
// ==========================================

void CAN_Init_Custom(FDCAN_HandleTypeDef *hfdcan) {
	FDCAN_FilterTypeDef sFilterConfig;

	sFilterConfig.IdType       = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex  = 0;
	sFilterConfig.FilterType   = FDCAN_FILTER_RANGE;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1    = 0x000;
	sFilterConfig.FilterID2    = 0x7FF;

	if (HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig) != HAL_OK) Error_Handler();

	if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) Error_Handler();

	if (HAL_FDCAN_Start(hfdcan) != HAL_OK) Error_Handler();
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
	FDCAN_RxHeaderTypeDef RxHeader;
	uint8_t RxData[8];

	if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0) {
		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK) {
			StoreDataCAN(&DICCP, RxHeader.Identifier, RxData);
		}
	}
}
