/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32c0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DfSUPled_Pin GPIO_PIN_3
#define DfSUPled_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
typedef struct {
	//Front ECU
	uint8_t FpDIGRpot;
	uint8_t FpDIGLpot;
	uint8_t FpDIGRvel;
	uint8_t FpDIGLvel;
	uint8_t FpANLbrake;
	uint8_t FpINTtsoff;
	uint8_t FpINTsbms;
	uint8_t FpINTr2d;
	uint8_t FpINTmenu;
	uint8_t FpDIGmicrosd;
	uint8_t FpSDCinertia;
	uint8_t FpSDCbots;
	uint8_t FpSDCcsdb;
	uint8_t FpERRapps;
	uint8_t FpDIGrefri;
	uint8_t FpDIGr2d;
	uint8_t FpINTrefrion;
	uint8_t FpDIGvel;
	uint16_t FpSHU;
	uint16_t FpANLRpot;

	//Rear ECU
	uint16_t RpSIGlvs;
	uint16_t RpSHU;
	uint8_t RpSDChvd;
	uint8_t RpSDCtsms;
	uint8_t RpSDClsdb;
	uint8_t RpSDCrsdb;
	uint8_t RpSTAbrkledR;
	uint8_t RpSTAbrkledG;
	uint8_t RpSTAbrkledB;
	uint8_t FpANLbrake;
	uint8_t FpDIGr2d;
	uint8_t FpINTrefrion;
	uint8_t IpRPM;
	uint8_t IpI;
	uint8_t IpPar;
	uint8_t IpV;
	uint8_t IpT_IGBT;
	uint8_t IpT_Mot;
	uint8_t IpErrL1;
	uint8_t IpErrH1;
	uint8_t IpErrL2;
	uint8_t IpErrH2;

	//TSALG
	uint8_t TpDIGspre;
	uint8_t TpDIGsairp;
	uint8_t TpDIGsairn;
	uint8_t TpDIGipre;
	uint8_t TpDIGiairp;
	uint8_t TpDIGiairn;
	uint8_t TpTHRhv;
	uint8_t TpERRscs;
	uint8_t TpTHRdis;
	uint8_t TpLCH;
	uint8_t TpINTled;

	//HVAB
	uint8_t ApTHRhv;
	uint16_t ApSHU;

	//HVDB
	uint8_t BpTHRbrake;
	uint8_t BpTHRcurrent;
	uint8_t BpERRplaus;
	uint8_t DpINTtsalred;
	uint8_t BpSDC;
	uint8_t DpSDC;
	uint8_t DpTHRhv;
	uint8_t DpLCHdischarge;
	uint8_t DpSDCintlck1;
	uint8_t DpSDCintlck2;
	uint16_t BpSHU;
	uint16_t DpSHU;

	//SDCReset
	uint8_t SpERRbms;
	uint8_t SpERRimd;
	uint8_t SpLCHebms;
	uint8_t SpLCHeimd;
	uint8_t SpINTresbut;
	uint8_t SpSDCbms;
	uint8_t SpSDCimd;
	uint16_t SpSHU;
} DICCP_t;

extern volatile DICCP_t DICCP;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
