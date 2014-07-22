#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "packet.h"
#include "fetcher.h"
#include "ui.h"

#define CONSOLE_LINES	15
#define CONSOLE_COLS	COLS
#define DISPLAY_LINES	(LINES - CONSOLE_LINES)
#define DISPLAY_COLS	COLS
#define DISPLAY_Y	0
#define DISPLAY_X	0
#define CONSOLE_Y	(DISPLAY_Y + DISPLAY_LINES)
#define CONSOLE_X	0

/* Global Variables */
HookRegisters *hook_head;

/* AArch64 identification registers */
ARMCPRegInfo v8_id[] = {
	/* Most ID registers are mapped to AArch32 registers.
	 * ex. MIDR_EL1 -> MIDR, ID_MMFR2_EL1 -> ID_MMFR2
	 * ref: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0500d/CHDIEHJC.html
	 */
	{ .name = "MIDR_EL1", .type = ARM_CP_CONST, .const_value = 0x411fd070},
	{ .name = "MPIDR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, MPIDR_EL1)},
	{ .name = "REVIDR_EL1", .type = ARM_CP_CONST, .const_value = 0x411fd070},
	{ .name = "ID_PFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00000131},
	{ .name = "ID_PFR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00011011},
	{ .name = "ID_DFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x03010066},
	{ .name = "ID_AFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ID_MMFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x10101105},
	{ .name = "ID_MMFR1_EL1", .type = ARM_CP_CONST, .const_value = 0x40000000},
	{ .name = "ID_MMFR2_EL1", .type = ARM_CP_CONST, .const_value = 0x01260000},
	{ .name = "ID_MMFR3_EL1", .type = ARM_CP_CONST, .const_value = 0x02102211},
	{ .name = "ID_ISAR0_EL1", .type = ARM_CP_CONST, .const_value = 0x02101110},
	{ .name = "ID_ISAR1_EL1", .type = ARM_CP_CONST, .const_value = 0x13112111},
	{ .name = "ID_ISAR2_EL1", .type = ARM_CP_CONST, .const_value = 0x21232042},
	{ .name = "ID_ISAR3_EL1", .type = ARM_CP_CONST, .const_value = 0x01112131},
	{ .name = "ID_ISAR4_EL1", .type = ARM_CP_CONST, .const_value = 0x00011142},
	{ .name = "ID_ISAR5_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ID_AA64PFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00002222},
	{ .name = "ID_AA64PFR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	/* XXX: QEMU dont' currently implement the PMU, so mask out the PMUVer field. */
	{ .name = "ID_AA64DFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x10305106 & ~0xf00},
	{ .name = "ID_AA64DFR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ID_AA64AFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ID_AA64AFR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ID_AA64ISAR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00010000},
	{ .name = "ID_AA64ISAR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ID_AA64MMFR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00001124},
	{ .name = "ID_AA64MMFR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "CCSIDR_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, CCSIDR_EL1)},
	{ .name = "CLIDR_EL1", .type = ARM_CP_CONST, .const_value = 0x0a200023},
	{ .name = "AIDR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "CSSELR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, CSSELR_EL1)},
	{ .name = "CTR_EL0", .type = ARM_CP_CONST, .const_value = 0x8444c004},
	{ .name = "DCZID_EL0", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, DCZID_EL0)},
	{ .name = "VPIDR_EL2"},
	{ .name = "VMPIDR_EL2"}
};

/* AArch64 exception handling registers */
ARMCPRegInfo v8_eh[] = {
	{ .name = "AFSR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "AFSR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ESR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, ESR_EL1)},
	{ .name = "IFSR32_EL2"},
	{ .name = "AFSR0_EL2"},
	{ .name = "AFSR1_EL2"},
	{ .name = "ESR_EL2"},
	{ .name = "AFSR0_EL3"},
	{ .name = "AFSR1_EL3"},
	{ .name = "ESR_EL3"},
	{ .name = "FAR_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, FAR_EL1)},
	{ .name = "FAR_EL2"},
	{ .name = "HPFAR_EL2"},
	{ .name = "FAR_EL3"},
	{ .name = "VBAR_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, VBAR_EL1)},
	{ .name = "ISR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, ISR_EL1)},
	{ .name = "VBAR_EL2", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, VBAR_EL2)},
	{ .name = "VBAR_EL3", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, VBAR_EL3)}
};

/* AArch64 virtual memory control registers  */
ARMCPRegInfo v8_vmc[] = {
	{ .name = "SCTLR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, SCTLR_EL1)},
	{ .name = "SCTLR_EL2"},
	{ .name = "SCTLR_EL3"},
	{ .name = "TTBR0_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, TTBR0_EL1)},
	{ .name = "TTBR1_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, TTBR1_EL1)},
	{ .name = "TCR_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, TCR_EL1)},
	{ .name = "TTBR0_EL2"},
	{ .name = "TCR_EL2"},
	{ .name = "VTTBR_EL2"},
	{ .name = "VTCR_EL2"},
	{ .name = "TTBR0_EL3"},
	{ .name = "TCR_EL3"},
	{ .name = "MAIR_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, MAIR_EL1)},
	{ .name = "AMAIR_EL1"},
	{ .name = "MAIR_EL2"},
	{ .name = "AMAIR_EL2"},
	{ .name = "MAIR_EL3"},
	{ .name = "AMAIR_EL3"},
	{ .name = "CONTEXTIDR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, CONTEXTIDR_EL1)}
};

/* AArch64 other system control registers */
ARMCPRegInfo v8_osc[] = {
	{ .name = "ACTLR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "CPACR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, CPACR_EL1)},
	{ .name = "ACTLR_EL2"},
	{ .name = "ACTLR_EL3"}
};

/* AArch64 performance monitor registers */
ARMCPRegInfo v8_pm[] = {
	{ .name = "PMCR_EL0", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, PMCR_EL0)},
	{ .name = "PMCNTENSET_EL0", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, PMCNTENSET_EL0)},
	{ .name = "PMCNTENCLR_EL0", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, PMCNTENCLR_EL0)},
	{ .name = "PMOVSCLR_EL0"},
	{ .name = "PMSWINC_EL0"},
	{ .name = "PMSELR_EL0", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "PMCEID0_EL0"},
	{ .name = "PMCEID1_EL0"},
	{ .name = "PMCCNTR_EL0"},
	{ .name = "PMXEVTYPER_EL0", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, PMXEVTYPER_EL0)},
	{ .name = "PMXEVCNTR_EL0", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "PMUSERENR_EL0", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "PMINTENSET_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, PMINTENSET_EL1)},
	{ .name = "PMINTENCLR_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, PMINTENCLR_EL1)},
	{ .name = "PMOVSSET_EL0"},
	{ .name = "PMEVCNTR0_EL0"},
	{ .name = "PMEVCNTR1_EL0"},
	{ .name = "PMEVCNTR2_EL0"},
	{ .name = "PMEVCNTR3_EL0"},
	{ .name = "PMEVCNTR4_EL0"},
	{ .name = "PMEVCNTR5_EL0"},
	{ .name = "PMEVTYPER0_EL0"},
	{ .name = "PMEVTYPER1_EL0"},
	{ .name = "PMEVTYPER2_EL0"},
	{ .name = "PMEVTYPER3_EL0"},
	{ .name = "PMEVTYPER4_EL0"},
	{ .name = "PMEVTYPER5_EL0"},
	{ .name = "PMCCFILTR_EL0"}
};

/* AArch64 reset registers */
ARMCPRegInfo v8_re[] = {
	{ .name = "RVBAR_EL3", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "RMR_EL3"}
};

/* AArch64 secure registers */
ARMCPRegInfo v8_se[] = {
	{ .name = "SCR_EL3"},
	{ .name = "SDER32_EL3"},
	{ .name = "CPTR_EL3"},
	{ .name = "MDCR_EL3"},
	{ .name = "AFSR0_EL3"},
	{ .name = "AFSR1_EL3"},
	{ .name = "VBAR_EL3", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, VBAR_EL3)}
};

/* AArch64 virtualization registers */
ARMCPRegInfo v8_vi[] = {
	{ .name = "VPIDR_EL2"},
	{ .name = "VMPIDR_EL2"},
	{ .name = "SCTLR_EL2"},
	{ .name = "ACTLR_EL2"},
	{ .name = "HCR_EL2"},
	{ .name = "MDCR_EL2"},
	{ .name = "CPTR_EL2"},
	{ .name = "HSTR_EL2"},
	{ .name = "HACR_EL2"},
	{ .name = "TTBR0_EL2"},
	{ .name = "TCR_EL2"},
	{ .name = "VTTBR_EL2"},
	{ .name = "VTCR_EL2"},
	{ .name = "DACR32_EL2"},
	{ .name = "AFSR0_EL2"},
	{ .name = "AFSR1_EL2"},
	{ .name = "ESR_EL2"},
	{ .name = "FAR_EL2"},
	{ .name = "HPFAR_EL2"},
	{ .name = "MAIR_EL2"},
	{ .name = "AMAIR_EL2"},
	{ .name = "VBAR_EL2"}
};

/* AArch64 GIC system registers */
ARMCPRegInfo v8_gic[] = {
	{ .name = "ICC_AP0R0_EL1"},
	{ .name = "ICC_AP1R0_EL1"},
	{ .name = "ICC_ASGI1R_EL1"},
	{ .name = "ICC_BPR0_EL1"},
	{ .name = "ICC_BPR1_EL1"},
	{ .name = "ICC_CTLR_EL1"},
	{ .name = "ICC_CTLR_EL3"},
	{ .name = "ICC_DIR_EL1"},
	{ .name = "ICC_EOIR0_EL1"},
	{ .name = "ICC_EOIR1_EL1"},
	{ .name = "ICC_HPPIR0_EL1"},
	{ .name = "ICC_HPPIR1_EL1"},
	{ .name = "ICC_IAR0_EL1"},
	{ .name = "ICC_IAR1_EL1"},
	{ .name = "ICC_IGRPEN0_EL1"},
	{ .name = "ICC_IGRPEN1_EL1"},
	{ .name = "ICC_IGRPEN1_EL3"},
	{ .name = "ICC_PMR_EL1"},
	{ .name = "ICC_RPR_EL1"},
	{ .name = "ICC_SEIEN_EL1"},
	{ .name = "ICC_SGI0R_EL1"},
	{ .name = "ICC_SGI1R_EL1"},
	{ .name = "ICC_SRE_EL1"},
	{ .name = "ICC_SRE_EL2"},
	{ .name = "ICC_SRE_EL3"}
};

/* AArch64 Generic Timer registers */
ARMCPRegInfo v8_gt[] = {
	{ .name = "CNTKCTL_EL1", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, CNTKCTL_EL1)},
	{ .name = "CNTFRQ_EL0", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, CNTFRQ_EL0)},
	{ .name = "CNTPCT_EL0"},
	{ .name = "CNTVCT_EL0"},
	{ .name = "CNTP_TVAL_EL0"},
	{ .name = "CNTP_CTL_EL0", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, CNTP_CTL_EL0)},
	{ .name = "CNTP_CVAL_EL0", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, CNTP_CVAL_EL0)},
	{ .name = "CNTV_TVAL_EL0"},
	{ .name = "CNTV_CTL_EL0", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, CNTV_CTL_EL0)},
	{ .name = "CNTV_CVAL_EL0", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, CNTV_CVAL_EL0)},
	{ .name = "CNTVOFF_EL2"},
	{ .name = "CNTHCTL_EL2"},
	{ .name = "CNTHP_TVAL_EL2"},
	{ .name = "CNTHP_CTL_EL2"},
	{ .name = "CNTHP_CVAL_EL2"},
	{ .name = "CNTPS_TVAL_EL1"},
	{ .name = "CNTPS_CTL_EL1"},
	{ .name = "CNTPS_CVAL_EL1"}
};

/* AArch64 thread registers */
ARMCPRegInfo v8_th[] = {
	{ .name = "TPIDR_EL0", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, TPIDR_EL0)},
	{ .name = "TPIDR_EL1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, TPIDR_EL1)},
	{ .name = "TPIDRRO_EL0", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, TPIDRRO_EL0)},
	{ .name = "TPIDR_EL2"},
	{ .name = "TPIDR_EL3"}
};

/* AArch64 implementation defined registers */
ARMCPRegInfo v8_imd[] = {
	{ .name = "ACTLR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "ACTLR_EL2"},
	{ .name = "ACTLR_EL3"},
	{ .name = "AFSR0_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "AFSR1_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "AFSR0_EL2"},
	{ .name = "AFSR1_EL2"},
	{ .name = "AFSR0_EL3"},
	{ .name = "AFSR1_EL3"},
	{ .name = "AMAIR_EL1"},
	{ .name = "AMAIR_EL2"},
	{ .name = "AMAIR_EL3"},
	{ .name = "L2CTLR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "L2ECTLR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "L2ACTLR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "CPUACTLR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "CPUECTLR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "CPUMERRSR_EL1", .type = ARM_CP_CONST, .const_value = 0x00000000},
	{ .name = "L2MERRSR_EL1"},
	{ .name = "CBAR_EL1"}
};

/* AArch64 address registers */
ARMCPRegInfo v8_ad[] = {
	{ .name = "PAR_EL1"}
};

/* AArch64 general purpose registers */
ARMCPRegInfo gpr[] = {
	{ .name = "x0", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[0])},
	{ .name = "x1", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[1])},
	{ .name = "x2", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[2])},
	{ .name = "x3", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[3])},
	{ .name = "x4", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[4])},
	{ .name = "x5", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[5])},
	{ .name = "x6", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[6])},
	{ .name = "x7", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[7])},
	{ .name = "x8", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[8])},
	{ .name = "x9", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[9])},
	{ .name = "x10", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[10])},
	{ .name = "x11", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[11])},
	{ .name = "x12", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[12])},
	{ .name = "x13", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[13])},
	{ .name = "x14", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[14])},
	{ .name = "x15", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[15])},
	{ .name = "x16", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[16])},
	{ .name = "x17", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[17])},
	{ .name = "x18", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[18])},
	{ .name = "x19", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[19])},
	{ .name = "x20", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[20])},
	{ .name = "x21", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[21])},
	{ .name = "x22", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[22])},
	{ .name = "x23", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[23])},
	{ .name = "x24", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[24])},
	{ .name = "x25", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[25])},
	{ .name = "x26", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[26])},
	{ .name = "x27", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[27])},
	{ .name = "x28", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[28])},
	{ .name = "x29", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[29])},
	{ .name = "x30", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[30])},
	{ .name = "x31", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, xregs[31])},
	{ .name = "pc", .type = ARM_CP_NORMAL_H, .fieldoffset = offsetof(FetcherPacket, pc)},
	{ .name = "spsr", .type = ARM_CP_NORMAL_L, .fieldoffset = offsetof(FetcherPacket, spsr)}
};

ARMCPRegArray reg_array[14] = {
	{ .name = "General Purpose Registers", .array = gpr, .size = sizeof(gpr) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 identification registers", .array = v8_id, .size = sizeof(v8_id) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 exception handling registers", .array = v8_eh, .size = sizeof(v8_eh) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 virtual memory control registers", .array = v8_vmc, .size = sizeof(v8_vmc) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 other system control registers", .array = v8_osc, .size = sizeof(v8_osc) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 performance monitor registers", .array = v8_pm, .size = sizeof(v8_pm) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 reset registers", .array = v8_re, .size = sizeof(v8_re) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 secure registers", .array = v8_se, .size = sizeof(v8_se) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 virtualization registers", .array = v8_vi, .size = sizeof(v8_vi) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 GIC system registers", .array = v8_gic, .size = sizeof(v8_gic) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 Generic Timer registers", .array = v8_gt, .size = sizeof(v8_gt) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 thread registers", .array = v8_th, .size = sizeof(v8_th) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 implementation definedregisters", .array = v8_imd, .size = sizeof(v8_imd) / sizeof(ARMCPRegInfo)},
	{ .name = "AArch64 address registers", .array = v8_ad, .size = sizeof(v8_ad) / sizeof(ARMCPRegInfo)}
};

/* UI Design
 * Split the whole window to two parts:
 *    1 - The upper part is used to print large information. ex. display registers
 *    2 - The lower part is command line(console) which interacts with user.
 * Upper part use maximum window height - 15, lower use remains.
 */

/* Display Design
 * Show display registers.
 */
WINDOW *display_win;
static void display_init()
{
	/* Create a window which size is LINES - 15 * COLS
	 * and with border around */
	display_win = newwin(DISPLAY_LINES, DISPLAY_COLS, DISPLAY_Y, DISPLAY_X);
	box(display_win, 0, 0);

	wrefresh(display_win);
}

void display_update(FetcherPacket packet)
{
	HookRegisters *it;
	int y = DISPLAY_Y + 1, x = DISPLAY_X + 1;

	wclear(display_win);
	box(display_win, 0, 0);

	for(it = hook_head; it != NULL; it = it->next) {
		ARMCPRegInfo tmp = reg_array[it->pos].array[it->index];
		mvwprintw(display_win, y, x, "%-16s = ", tmp.name);
		x += 19;
		switch(tmp.type) {
		case ARM_CP_UNIMPL:
			mvwprintw(display_win, y, x, "UNIMPLEMENTED");
			break;
		case ARM_CP_CONST:
			mvwprintw(display_win, y, x, "0x%lx", tmp.const_value);
			break;
		case ARM_CP_NORMAL_L:
			mvwprintw(display_win, y, x, "0x%x", *(uint32_t *)((uint8_t *)(&packet) + tmp.fieldoffset));
			break;
		case ARM_CP_NORMAL_H:
			mvwprintw(display_win, y, x, "0x%lx", *(uint64_t *)((uint8_t *)(&packet) + tmp.fieldoffset));
			break;
		}
		y++;
		x = DISPLAY_X + 1;
	}

	wrefresh(display_win);
}

/* Free dynamic allocate memory */
static void display_destructor(void)
{
	HookRegisters *it, *pre;

	/* Do not have hook register */
	if(hook_head == NULL) {
		return;
	}

	pre = hook_head;
	it = hook_head->next;
	while(pre) {
		free(pre);
		pre = it;
		if(it) {
			it = it->next;
		}
	}
}

/* Add hook registers which need to be trace */
void display_add(char *input)
{
	HookRegisters *it = hook_head;
	int i, j;
	int valid = 0;

	/* Search register in registers array */
	for(i = 0; i < sizeof(reg_array) / sizeof(struct ARMCPRegArray); i++) {
		for(j = 0; j < reg_array[i].size; j++) {
			if(!strcmp(reg_array[i].array[j].name, input)) {
				valid = 1;
				break;
			}
		}

		if(valid) {
			break;
		}
	}

	if(!valid) {
		console_puts("Invalid register name\n");
		return;
	}

	HookRegisters *tmp = (HookRegisters *)malloc(sizeof(HookRegisters));
	tmp->pos = i;
	tmp->index = j;
	tmp->next = NULL;
	console_puts("Add register ");
	console_puts(input);
	console_puts(" to hook list\n");

	/* First element */
	if(it == NULL) {
		hook_head = tmp;
		return;
	}

	/* Iterate to last element */
	for(; it != NULL; it = it->next) {
		if(!strcmp(reg_array[it->pos].array[it->index].name, input)) {
			console_puts("Register ");
			console_puts(input);
			console_puts(" has already in hook list\n");
			return;
		}

		if(it->next == NULL) {
			it->next = tmp;
			break;
		}
	}
}


/* Console Design
 * We need to handle console ourself. Try to make it like normal stdout act.
 * Provide some API for programmer use:
 *    * putc - put a character on console
 */
WINDOW *console_win;
static void console_init()
{
	console_win = newwin(CONSOLE_LINES, CONSOLE_COLS, CONSOLE_Y, CONSOLE_X);
	wrefresh(console_win);
}

static void console_putc(char c)
{
	static int y = 0, x = 0; // Record current cursor pos

	/* Manipulate output character */
	switch(c) {
	case '\n':
		y++;
		x = 0;
		break;
	default:
		mvwaddch(console_win, y, x, c);
		x++;
	}

	/* Manipulate cursor postion */
	if(x >= CONSOLE_COLS) { // reach column limit, new line
		x = 0;
		y++;
	}
	if(y >= CONSOLE_LINES - 1) { // reach line limit, clear console
		/* Clear page notice */
		mvwaddstr(console_win, y, 0, "---- Type any key to continue ----");
		wrefresh(console_win);
		getchar();
		wclear(console_win);

		x = 0;
		y = 0;
	}

	wrefresh(console_win);
}

void console_puts(char *str)
{
	char *cptr;

	for(cptr = str; *cptr != '\0'; cptr++) {
		console_putc(*cptr);
	}
}

static void console_parser(char *str)
{
	char *pch;

	/* First level parse */
	pch = strtok(str, " \n");

	if(!strcmp(pch, "add")) {
		pch = strtok(NULL, " \n");
		display_add(pch);
	}
	else {
		console_puts("Invalid command!\n");
	}
}

void console_prompt(void)
{
	char line_buf[128];
	char c;
	int count = 0;

	console_puts("-> ");
	while(1) {
		c = getch();
		if(c == '\n') {
			console_putc(c);
			line_buf[count++] = '\0';
			if(count > 1) {
				console_parser(line_buf);
			}
			count = 0;
			console_puts("-> ");
		}
		/* FIXME can not recongize backspace */
		else if(c == 127 || c == 8) {
			console_putc('b');
			count--;
		}
		else {
			console_putc(c);
			line_buf[count++] = c;
		}
	}
}

/* UI initiallize and destructor */
void ui_init(void)
{
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	refresh();

	display_init();
	console_init();
}

void ui_destroy(void)
{
	display_destructor();
	endwin();
}
