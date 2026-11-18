/*
 * cpf.h
 *
 * Format de paquets CPF de l'ES_CANEMviewer_SW (branca developement, README.md):
 *   - ID de la trama:  ULEB128, màxim 0x1FFF_FFFF
 *   - Timestamp (ms):  ULEB128, màxim 0xFFFF_FFFF
 *   - Longitud:        1 byte, 0..8
 *   - Dades:           0..8 bytes
 * El fitxer no té capçalera: només paquets un darrere l'altre.
 *
 * El buffer circular té un sol productor (la interrupció de CAN) i un sol
 * consumidor (el bucle principal), així que no cal desactivar interrupcions.
 */

#ifndef INC_CPF_H_
#define INC_CPF_H_

#include <stdint.h>

#define CPF_MAX_ID          0x1FFFFFFFUL
#define CPF_MAX_DATA_LEN    8U
#define CPF_MAX_PACKET_SIZE 19U  /* 5 + 5 + 1 + 8 */

/* Mida del buffer circular en bytes. Ha de ser potència de 2. */
#ifndef CPF_RING_SIZE
#define CPF_RING_SIZE 4096U
#endif

/*
 * Codifica un paquet a out (com a mínim CPF_MAX_PACKET_SIZE bytes).
 * Retorna els bytes escrits, o 0 si l'ID o la longitud no són vàlids.
 */
uint32_t CPF_EncodePacket(uint8_t *out, uint32_t id, uint32_t timestamp_ms,
                          const uint8_t *data, uint32_t len);

/* Buffer circular de paquets codificats */

/* Buida el buffer. Només s'ha de cridar quan el productor no hi escriu. */
void CPF_Ring_Reset(void);

/*
 * Afegeix un paquet sencer (productor). Retorna 1 si hi cabia, 0 si no
 * (el paquet no s'escriu, ni parcialment).
 */
int CPF_Ring_Push(uint32_t id, uint32_t timestamp_ms,
                  const uint8_t *data, uint32_t len);

/* Bytes pendents de llegir (consumidor) */
uint32_t CPF_Ring_Available(void);

/*
 * Bytes contigus que es poden llegir a partir de *chunk sense donar la volta
 * al buffer (consumidor). No consumeix res; cal cridar CPF_Ring_Consume.
 */
uint32_t CPF_Ring_Peek(const uint8_t **chunk);

/* Marca n bytes com a llegits (consumidor) */
void CPF_Ring_Consume(uint32_t n);

#endif /* INC_CPF_H_ */
