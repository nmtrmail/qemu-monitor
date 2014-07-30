#ifndef __PACKET_H_
#define __PACKET_H_

#include <stdint.h>
#include <stddef.h>

/* Packet format from QEMU */
typedef struct FetcherPacket {
	/* AArch64 System Registers */
	uint32_t MPIDR_EL1; /* cpu->cpu_index */
	uint64_t CCSIDR_EL1; /* cpu->ccsidr[env->cp15.c0_cssel] */
	uint32_t CSSELR_EL1; /* env->cp15.c10_cssel */
	uint32_t DCZID_EL0; /* cpu->dcz_blocksize, check target-arm/helper.c:1842 */
	uint32_t ESR_EL1; /* env->cp15.esr_el[1] */
	uint64_t FAR_EL1; /* env->cp15.far_el1 */
	uint64_t VBAR_EL1; /* env->cp15.vbar_el[1] */
	uint32_t ISR_EL1; /* check target-arm/helper.c:699 */
	uint64_t VBAR_EL2; /* env->cp15.vbar_el[2] */
	uint64_t VBAR_EL3; /* env->cp15.vbar_el[3] */
	uint32_t SCTLR_EL1; /* env->cp15.c1_sys */
	uint64_t TTBR0_EL1; /* env->cp15.ttbr0_el1 */
	uint64_t TTBR1_EL1; /* env->cp15.ttbr1_el1 */
	uint64_t TCR_EL1; /* env->cp15.c2_control */
	uint64_t MAIR_EL1; /* env->cp15.mair_el1 */
	uint32_t CONTEXTIDR_EL1; /* env->cp15.contextidr_el1 */
	uint32_t CPACR_EL1; /* env->cp15.c1_coproc */

	uint32_t PMCR_EL0; /* env->cp15.c9_pmcr */
	uint32_t PMCNTENSET_EL0; /* env->cp15.c9_pmcnten */
	uint32_t PMCNTENCLR_EL0; /* env->cp15.c9_pmcnten */
	uint32_t PMXEVTYPER_EL0; /* env->cp15.c9_pmxevtyper */
	uint32_t PMUSERENR_EL0; /* env->cp15.c9_pmuserenr */
	uint32_t PMINTENSET_EL1; /* env->cp15.c9_pminten */
	uint32_t PMINTENCLR_EL1; /* env->cp15.c9_pminten */

	uint32_t CNTKCTL_EL1; /* env->cp15.c14_cntkctl */
	uint64_t CNTFRQ_EL0; /* env->cp15.c14_cntfrq */
	uint32_t CNTP_CTL_EL0; /* env->cp15.c14_timer[GTIMER_PHYS].ctl */
	uint64_t CNTP_CVAL_EL0; /* env->cp15.c14_timer[GTIMER_PHYS].cval */
	uint32_t CNTV_CTL_EL0; /* env->cp15.c14_timer[GTIMER_VIRT].ctl */
	uint64_t CNTV_CVAL_EL0; /* env->cp15.c14_timer[GTIMER_VIRT].cval */

	uint64_t TPIDR_EL0; /* env->cp15.tpidr_el0 */
	uint64_t TPIDR_EL1; /* env->cp15.tpidr_el1 */
	uint64_t TPIDRRO_EL0; /* env->cp15.tpidrro_el0 */

	/* General Purpose Registers */
	uint64_t xregs[32];
	uint64_t pc;

	/* For AArch64 from AArch64, not support from AArch32 */
	union {
		struct {
			uint32_t N : 1;
			uint32_t Z : 1;
			uint32_t C : 1;
			uint32_t V : 1;
			uint32_t RES0_1 : 4;
			uint32_t RES0_2 : 2;
			uint32_t SS : 1;
			uint32_t IL : 1;
			uint32_t RES0_3 : 10;
			uint32_t D : 1;
			uint32_t A : 1;
			uint32_t I : 1;
			uint32_t F : 1;
			uint32_t RES0_4 : 1;
			/* M[4], Execution state */
			uint32_t Mb4 : 1;
			/* M[3:0] */
			uint32_t Mb3 : 1;
			uint32_t Mb2 : 1;
			uint32_t Mb1 : 1;
			uint32_t Mb0 : 1;
		} spsr_s;
		uint32_t spsr_t;
	} spsr;
} FetcherPacket;

#endif
