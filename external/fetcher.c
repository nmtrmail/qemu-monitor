#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>


#include "fetcher.h"
#include "packet.h"
#include "cpu.h"

#define ADDRESS "fetcher"

static int ns = 0;

/* Used for unused parameters to silence gcc warnings */
#define UNUSED __attribute__((__unused__))

void fetcher_start(void)
{
        int s, len;
	socklen_t fromlen UNUSED;

	fromlen = 0;

        struct sockaddr_un saun, fsaun;
        if((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
                perror("Server: Socket");
                exit(1);
        }

        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, ADDRESS);

        unlink(ADDRESS);
        len = sizeof(saun.sun_family) + strlen(saun.sun_path);

        if(bind(s, &saun, len) < 0) {
                perror("Server: Bind");
                exit(1);
        }

        if(listen(s, 5) < 0) {
                perror("Server: Listen");
                exit(1);
        }

        if((ns = accept(s, &fsaun, &fromlen)) < 0) {
                perror("Server: Accept");
                exit(1);
        }
}

static void copy_register(FetcherPacket *dst, CPUState *cs)
{
	int i;
	ARMCPU *cpu = ARM_CPU(cs);
	CPUARMState *env = &cpu->env;

	/* Copy System Control Registers */
	dst->MPIDR_EL1 = cs->cpu_index;
	dst->CCSIDR_EL1 = cpu->ccsidr[env->cp15.c0_cssel];
	dst->CSSELR_EL1 = env->cp15.c0_cssel;
	dst->DCZID_EL0 = cpu->dcz_blocksize | (1 << 4);
	dst->ESR_EL1 = env->cp15.esr_el[1];
	dst->FAR_EL1 = env->cp15.far_el1;
	dst->VBAR_EL1 = env->cp15.vbar_el[1];
	dst->ISR_EL1 = 0;
	if(cs->interrupt_request & 0x0002) { // XXX: Hardcode CPU_INTERRUPT_HARD
		dst->ISR_EL1 |= CPSR_I;
	}
	if(cs->interrupt_request & 0x0010) { // XXX: Hardcode CPU_INTERRUPT_FIQ
		dst->ISR_EL1 |= CPSR_F;
	}
	dst->VBAR_EL2 = env->cp15.vbar_el[2];
	dst->VBAR_EL3 = env->cp15.vbar_el[3];
	dst->SCTLR_EL1 = env->cp15.c1_sys;
	dst->TTBR0_EL1 = env->cp15.ttbr0_el1;
	dst->TTBR1_EL1 = env->cp15.ttbr1_el1;
	dst->TCR_EL1 = env->cp15.c2_control;
	dst->MAIR_EL1 = env->cp15.mair_el1;
	dst->CONTEXTIDR_EL1 = env->cp15.contextidr_el1;
	dst->CPACR_EL1 = env->cp15.c1_coproc;

	dst->PMCR_EL0 = env->cp15.c9_pmcr;
	dst->PMCNTENSET_EL0 = env->cp15.c9_pmcnten;
	dst->PMCNTENCLR_EL0 = env->cp15.c9_pmcnten;
	dst->PMXEVTYPER_EL0 = env->cp15.c9_pmxevtyper;
	dst->PMUSERENR_EL0 = env->cp15.c9_pmuserenr;
	dst->PMINTENSET_EL1 = env->cp15.c9_pminten;
	dst->PMINTENCLR_EL1 = env->cp15.c9_pminten;

	dst->CNTKCTL_EL1 = env->cp15.c14_cntkctl;
	dst->CNTFRQ_EL0 = env->cp15.c14_cntfrq;
	dst->CNTP_CTL_EL0 = env->cp15.c14_timer[GTIMER_PHYS].ctl;
	dst->CNTP_CVAL_EL0 = env->cp15.c14_timer[GTIMER_PHYS].cval;
	dst->CNTV_CTL_EL0 = env->cp15.c14_timer[GTIMER_VIRT].ctl;
	dst->CNTV_CVAL_EL0 = env->cp15.c14_timer[GTIMER_VIRT].cval;

	dst->TPIDR_EL0 = env->cp15.tpidr_el0;
	dst->TPIDR_EL1 = env->cp15.tpidr_el1;
	dst->TPIDRRO_EL0 = env->cp15.tpidrro_el0;

	/* Copy xregs */
	dst->pc = env->pc;
	for(i = 0; i < 32; i++) {
		dst->xregs[i] = env->xregs[i];
	}

	/* Copy SPSR */
	dst->spsr.spsr_t = (env->NF & 0x80000000) | ((env->ZF == 0) << 30)
	            | (env->CF << 29) | ((env->VF & 0x80000000) >> 3)
		    | env->pstate | env->daif;
}

void fetcher_trans(CPUState *cs)
{
	static FetcherPacket packet;
	copy_register(&packet, cs);

	send(ns, &packet, sizeof(FetcherPacket), 0);
}
