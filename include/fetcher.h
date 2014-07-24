#ifndef __FETCHER_H_
#define __FETCHER_H_

#include <stdint.h>
#include <stddef.h>

#include "packet.h"

/* ARMCPRegInfo state: unimplemented in qemu, constant, normal uint32, normal uint64*/
#define ARM_CP_UNIMPL	0
#define ARM_CP_CONST 	1
#define ARM_CP_NORMAL_L	2
#define ARM_CP_NORMAL_H	3

/* XXX: The constant value in ARMCPRegInfo is implementation-dependent, and
 * now is mapped to aarch64_a57 in `qemu/target-arm/cpu64.c`.
 */
typedef struct ARMCPRegInfo {
	/* Name of register */
	const char *name;
	/* register type */
	uint8_t type;
	/* offset in packet */
	ptrdiff_t fieldoffset;
	/* const type value */
	uint64_t const_value;
} ARMCPRegInfo;


/* Combine all register array */
typedef struct ARMCPRegArray {
	const char *name;
	ARMCPRegInfo *array;
	int size;
} ARMCPRegArray;

/* Record registers which need to be display every step
 * pos : first level
 * index : second level
 * ex. reg_array[pos].array[index].name
 */
typedef struct HookRegisters {
	int pos;
	int index;
	struct HookRegisters *next;
} HookRegisters;


#endif
