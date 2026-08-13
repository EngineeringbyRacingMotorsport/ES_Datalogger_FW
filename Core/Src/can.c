#include <can.h>

void CAN_Init_Custom(FDCAN_HandleTypeDef *hfdcan) {
	FDCAN_FilterTypeDef sFilterConfig;

	// 1. Configuració de filtre per acceptar-ho TOT
	sFilterConfig.IdType = FDCAN_STANDARD_ID;
	sFilterConfig.FilterIndex = 0;
	sFilterConfig.FilterType = FDCAN_FILTER_RANGE;
	sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	sFilterConfig.FilterID1 = 0x100;
	sFilterConfig.FilterID2 = 0x600;

	if (HAL_FDCAN_ConfigFilter(hfdcan, &sFilterConfig) != HAL_OK) Error_Handler();

	// 2. Activar la interrupció de la FIFO 0
	if (HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) Error_Handler();

	// 3. Arrencar el perifèric
	if (HAL_FDCAN_Start(hfdcan) != HAL_OK) Error_Handler();
}

static void StoreDataCAN(volatile DICCP_t *DICCP, uint32_t can_id, uint8_t *data) {
	switch (can_id)
	{

	//-----------------------------------------------------------------------FrontECU

	case 0x100:
		DICCP->FpDIGRpot = data[0];
		DICCP->FpDIGLpot = data[1];
		DICCP->FpDIGRvel = data[2];
		DICCP->FpDIGLvel = data[3];
		DICCP->FpANLbrake = data[4];
		break;

	case 0x101:
		DICCP->FpINTtsoff = (data[0] >> 0) & 0x01;
		DICCP->FpINTsbms = (data[0] >> 1) & 0x01;
		DICCP->FpINTr2d = (data[0] >> 2) & 0x01;
		DICCP->FpINTmenu = (data[0] >> 3) & 0x01;
		DICCP->FpDIGmicrosd = (data[0] >> 4) & 0x01;

		DICCP->FpSDCinertia = (data[1] >> 0) & 0x01;
		DICCP->FpSDCbots = (data[1] >> 1) & 0x01;
		DICCP->FpSDCcsdb = (data[1] >> 2) & 0x01;
		DICCP->FpERRapps = (data[1] >> 3) & 0x01;
		DICCP->FpDIGrefri = (data[1] >> 4) & 0x01;
		DICCP->FpDIGr2d = (data[1] >> 5) & 0x01;
		DICCP->FpINTrefrion = (data[1] >> 6) & 0x01;

		DICCP->FpDIGvel = data[2];

		DICCP->FpSHU = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
		break;

	case 0x102:
		DICCP->FpANLRpot = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
		break;

		//-----------------------------------------------------------------------RearECU

	case 0x200:
		DICCP->RpSDChvd = (data[0] >> 0) & 0x01;
		DICCP->RpSDCtsms = (data[0] >> 1) & 0x01;
		DICCP->RpSDCrsdb = (data[0] >> 2) & 0x01;
		DICCP->RpSDClsdb = (data[0] >> 3) & 0x01;
		DICCP->RpSTAbrkledR = (data[0] >> 4) & 0x01;
		DICCP->RpSTAbrkledG = (data[0] >> 5) & 0x01;
		DICCP->RpSTAbrkledB = (data[0] >> 6) & 0x01;
		DICCP->RpSIGlvs = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
		DICCP->RpSHU = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
		break;

	case 0x201:
		DICCP->IpRPM = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
		DICCP->IpI = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
		DICCP->IpV = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
		DICCP->IpPar = (uint16_t)data[6] | ((uint16_t)data[7] << 8);
		break;

	case 0x202:
		DICCP->IpT_IGBT = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
		DICCP->IpT_Mot = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
		DICCP->IpErrL1 = data[4];
		DICCP->IpErrH1 = data[5];
		DICCP->IpErrL2 = data[6];
		DICCP->IpErrH2 = data[7];
		break;

		//-----------------------------------------------------------------------HVAB

	case 0x300:
		DICCP->ApTHRhv = (data[0] >> 0) & 0x01;
		DICCP->ApSHU = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
		break;

		//-----------------------------------------------------------------------HVDB

	case 0x400:
		DICCP->BpTHRbrake = (data[0] >> 0) & 0x01;
		DICCP->BpTHRcurrent = (data[0] >> 1) & 0x01;
		DICCP->BpERRplaus = (data[0] >> 2) & 0x01;
		DICCP->DpINTtsalred = (data[0] >> 3) & 0x01;
		DICCP->BpSDC = (data[0] >> 4) & 0x01;
		DICCP->DpSDC = (data[1] >> 0) & 0x01;
		DICCP->DpTHRhv = (data[1] >> 1) & 0x01;
		DICCP->DpLCHdischarge = (data[1] >> 2) & 0x01;
		DICCP->DpSDCintlck1 = (data[1] >> 3) & 0x01;
		DICCP->DpSDCintlck2 = (data[1] >> 4) & 0x01;
		DICCP->BpSHU = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
		DICCP->DpSHU = (uint16_t)data[4] | ((uint16_t)data[5] << 8);

		//-----------------------------------------------------------------------TSALG

	case 0x500:
		DICCP->TpDIGspre = (data[0] >> 0) & 0x01;
		DICCP->TpDIGsairp = (data[0] >> 1) & 0x01;
		DICCP->TpDIGsairn = (data[0] >> 2) & 0x01;
		DICCP->TpDIGipre = (data[0] >> 3) & 0x01;
		DICCP->TpDIGiairp = (data[0] >> 4) & 0x01;
		DICCP->TpDIGiairn = (data[0] >> 5) & 0x01;
		DICCP->TpTHRhv = (data[1] >> 0) & 0x01;
		DICCP->TpERRscs = (data[2] >> 1) & 0x01;
		DICCP->TpTHRdis = (data[3] >> 2) & 0x01;
		DICCP->TpLCH = (data[4] >> 3) & 0x01;
		DICCP->TpINTled = (data[5] >> 4) & 0x01;
		break;

		//-----------------------------------------------------------------------SDCReset

	case 0x600:
		DICCP->SpERRbms = (data[0] >> 0) & 0x01;
		DICCP->SpERRimd = (data[0] >> 1) & 0x01;
		DICCP->SpLCHebms = (data[0] >> 2) & 0x01;
		DICCP->SpLCHeimd = (data[0] >> 3) & 0x01;
		DICCP->SpINTresbut = (data[0] >> 4) & 0x01;
		DICCP->SpSDCbms = (data[0] >> 5) & 0x01;
		DICCP->SpSDCimd = (data[0] >> 6) & 0x01;
		DICCP->SpSHU = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
		break;

	default:
		break;
	}
}

extern volatile DICCP_t DICCP;

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
	FDCAN_RxHeaderTypeDef RxHeader;
	uint8_t RxData[8];

	if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0)
	{
		// 2. Extraer el mensaje del FIFO hardware
		if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
		{
			StoreDataCAN(&DICCP, RxHeader.Identifier, RxData);
		}
	}
}
