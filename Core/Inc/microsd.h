#ifndef MICROSD_H
#define MICROSD_H

#include "main.h"
#include <stdint.h>

// Prototips de funcions
int MicroSD_Init(void);
int MicroSD_LogData(volatile DICCP_t *DICCP);
void MicroSD_Unmount(void);

#endif // MICROSD_H
