/*
 * cpf.c
 *
 * Codificació de paquets CPF i buffer circular entre la interrupció de CAN i
 * el bucle principal. No depèn de la HAL per poder-ho provar a l'ordinador.
 */

#include "cpf.h"
#include <string.h>

#if (CPF_RING_SIZE & (CPF_RING_SIZE - 1U)) != 0U
#error "CPF_RING_SIZE ha de ser potencia de 2"
#endif

/* Barrera perquè les dades s'escriguin abans de publicar l'índex */
#if defined(__arm__)
#define CPF_BARRIER() __asm volatile ("dmb" ::: "memory")
#else
#define CPF_BARRIER() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#endif

static uint8_t ring[CPF_RING_SIZE];
/* Índexs que només creixen; la posició és índex & (CPF_RING_SIZE - 1) */
static volatile uint32_t ring_head = 0;  /* només l'escriu el productor */
static volatile uint32_t ring_tail = 0;  /* només l'escriu el consumidor */

static uint32_t Write_ULEB128(uint8_t *out, uint32_t value) {
	uint32_t n = 0;
	do {
		uint8_t byte = (uint8_t)(value & 0x7FU);
		value >>= 7;
		if (value != 0U) {
			byte |= 0x80U;
		}
		out[n++] = byte;
	} while (value != 0U);
	return n;
}

uint32_t CPF_EncodePacket(uint8_t *out, uint32_t id, uint32_t timestamp_ms,
                          const uint8_t *data, uint32_t len) {
	if (id > CPF_MAX_ID || len > CPF_MAX_DATA_LEN) {
		return 0;
	}

	uint32_t n = Write_ULEB128(out, id);
	n += Write_ULEB128(out + n, timestamp_ms);
	out[n++] = (uint8_t)len;
	if (len > 0U) {
		memcpy(out + n, data, len);
		n += len;
	}
	return n;
}

void CPF_Ring_Reset(void) {
	ring_tail = ring_head;
}

int CPF_Ring_Push(uint32_t id, uint32_t timestamp_ms,
                  const uint8_t *data, uint32_t len) {
	uint8_t packet[CPF_MAX_PACKET_SIZE];
	uint32_t size = CPF_EncodePacket(packet, id, timestamp_ms, data, len);
	if (size == 0U) {
		return 0;
	}

	uint32_t head = ring_head;
	uint32_t free_space = CPF_RING_SIZE - (head - ring_tail);
	if (size > free_space) {
		return 0;
	}

	uint32_t pos = head & (CPF_RING_SIZE - 1U);
	uint32_t first = CPF_RING_SIZE - pos;
	if (first > size) {
		first = size;
	}
	memcpy(&ring[pos], packet, first);
	memcpy(&ring[0], packet + first, size - first);

	CPF_BARRIER();
	ring_head = head + size;
	return 1;
}

uint32_t CPF_Ring_Available(void) {
	return ring_head - ring_tail;
}

uint32_t CPF_Ring_Peek(const uint8_t **chunk) {
	uint32_t tail = ring_tail;
	uint32_t available = ring_head - tail;
	CPF_BARRIER();

	uint32_t pos = tail & (CPF_RING_SIZE - 1U);
	uint32_t contiguous = CPF_RING_SIZE - pos;
	*chunk = &ring[pos];
	return (available < contiguous) ? available : contiguous;
}

void CPF_Ring_Consume(uint32_t n) {
	CPF_BARRIER();
	ring_tail = ring_tail + n;
}
