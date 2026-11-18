#ifndef MICROSD_H
#define MICROSD_H

#include "main.h"
#include <stdint.h>

/* Trames que no s'han pogut guardar perquè el buffer estava ple */
extern volatile uint32_t MicroSD_DroppedFrames;

// Prototips de funcions
int MicroSD_Init(void);
void MicroSD_OnFrame(uint32_t id, const uint8_t *data, uint32_t len);
void MicroSD_Task(void);
void MicroSD_Unmount(void);

#endif // MICROSD_H
