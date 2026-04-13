/*
 * rpiz - https://github.com/bp0/rpiz
 * Copyright (C) 2017  Burt P. <pburt0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <json-glib/json-glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "x86_data.h"

#ifndef C_
#define C_(Ctx, String) String
#endif
#ifndef NC_
#define NC_(Ctx, String) String
#endif

/* sources:
 *   https://unix.stackexchange.com/a/43540
 *   https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git/tree/arch/x86/include/asm/cpufeatures.h?id=refs/tags/v4.9
 *   hardinfo: modules/devices/x86/processor.c
 */

struct flag_to_meaning {
    char *name;
    char *meaning;
};

static const struct flag_to_meaning builtin_tab_flag_meaning[] = {
/* Intel-defined CPU features, CPUID level 0x00000001 (edx)
 * See also Wikipedia and table 2-27 in Intel Advanced Vector Extensions Programming Reference */
    { "fpu",     NC_("x86-flag", /*!/flag:fpu*/  "Onboard FPU (floating point support)") },
    { "vme",     NC_("x86-flag", /*!/flag:vme*/  "Virtual 8086 mode enhancements") },
    { "de",      NC_("x86-flag", /*!/flag:de*/  "Debugging Extensions (CR4.DE)") },
    { "pse",     NC_("x86-flag", /*!/flag:pse*/  "Page Size Extensions (4MB memory pages)") },
    { "tsc",     NC_("x86-flag", /*!/flag:tsc*/  "Time Stamp Counter (RDTSC)") },
    { "msr",     NC_("x86-flag", /*!/flag:msr*/  "Model-Specific Registers (RDMSR, WRMSR)") },
    { "pae",     NC_("x86-flag", /*!/flag:pae*/  "Physical Address Extensions (support for more than 4GB of RAM)") },
    { "mce",     NC_("x86-flag", /*!/flag:mce*/  "Machine Check Exception") },
    { "cx8",     NC_("x86-flag", /*!/flag:cx8*/  "CMPXCHG8 instruction (64-bit compare-and-swap)") },
    { "apic",    NC_("x86-flag", /*!/flag:apic*/  "Onboard APIC") },
    { "sep",     NC_("x86-flag", /*!/flag:sep*/  "SYSENTER/SYSEXIT") },
    { "mtrr",    NC_("x86-flag", /*!/flag:mtrr*/  "Memory Type Range Registers") },
    { "pge",     NC_("x86-flag", /*!/flag:pge*/  "Page Global Enable (global bit in PDEs and PTEs)") },
    { "mca",     NC_("x86-flag", /*!/flag:mca*/  "Machine Check Architecture") },
    { "cmov",    NC_("x86-flag", /*!/flag:cmov*/  "CMOV instructions (conditional move) (also FCMOV)") },
    { "pat",     NC_("x86-flag", /*!/flag:pat*/  "Page Attribute Table") },
    { "pse36",   NC_("x86-flag", /*!/flag:pse36*/  "36-bit PSEs (huge pages)") },
    { "pn",      NC_("x86-flag", /*!/flag:pn*/  "Processor serial number") },
    { "clflush", NC_("x86-flag", /*!/flag:clflush*/  "Cache Line Flush instruction") },
    { "dts",     NC_("x86-flag", /*!/flag:dts*/  "Debug Store (buffer for debugging and profiling instructions), or alternately: digital thermal sensor") },
    { "acpi",    NC_("x86-flag", /*!/flag:acpi*/  "ACPI via MSR (temperature monitoring and clock speed modulation)") },
    { "mmx",     NC_("x86-flag", /*!/flag:mmx*/  "Multimedia Extensions") },
    { "fxsr",    NC_("x86-flag", /*!/flag:fxsr*/  "FXSAVE/FXRSTOR, CR4.OSFXSR") },
    { "sse",     NC_("x86-flag", /*!/flag:sse*/  "Intel SSE vector instructions") },
    { "sse2",    NC_("x86-flag", /*!/flag:sse2*/  "SSE2") },
    { "ss",      NC_("x86-flag", /*!/flag:ss*/  "CPU self snoop") },
    { "ht",      NC_("x86-flag", /*!/flag:ht*/  "Hyper-Threading") },
    { "tm",      NC_("x86-flag", /*!/flag:tm*/  "Automatic clock control (Thermal Monitor)") },
    { "ia64",    NC_("x86-flag", /*!/flag:ia64*/  "Intel Itanium Architecture 64-bit (not to be confused with Intel's 64-bit x86 architecture with flag x86-64 or \"AMD64\" bit indicated by flag lm)") },
    { "pbe",     NC_("x86-flag", /*!/flag:pbe*/  "Pending Break Enable (PBE# pin) wakeup support") },
/* AMD-defined CPU features, CPUID level 0x80000001
 * See also Wikipedia and table 2-23 in Intel Advanced Vector Extensions Programming Reference */
    { "syscall",  NC_("x86-flag", /*!/flag:syscall*/  "SYSCALL (Fast System Call) and SYSRET (Return From Fast System Call)") },
    { "mp",       NC_("x86-flag", /*!/flag:mp*/  "Multiprocessing Capable.") },
    { "nx",       NC_("x86-flag", /*!/flag:nx*/  "Execute Disable") },
    { "mmxext",   NC_("x86-flag", /*!/flag:mmxext*/  "AMD MMX extensions") },
    { "fxsr_opt", NC_("x86-flag", /*!/flag:fxsr_opt*/  "FXSAVE/FXRSTOR optimizations") },
    { "pdpe1gb",  NC_("x86-flag", /*!/flag:pdpe1gb*/  "One GB pages (allows hugepagesz=1G)") },
    { "rdtscp",   NC_("x86-flag", /*!/flag:rdtscp*/  "Read Time-Stamp Counter and Processor ID") },
    { "lm",       NC_("x86-flag", /*!/flag:lm*/  "Long Mode (x86-64: amd64, also known as Intel 64, i.e. 64-bit capable)") },
    { "3dnow",    NC_("x86-flag", /*!/flag:3dnow*/  "3DNow! (AMD vector instructions, competing with Intel's SSE1)") },
    { "3dnowext", NC_("x86-flag", /*!/flag:3dnowext*/  "AMD 3DNow! extensions") },
/* Transmeta-defined CPU features, CPUID level 0x80860001 */
    { "recovery", NC_("x86-flag", /*!/flag:recovery*/  "CPU in recovery mode") },
    { "longrun",  NC_("x86-flag", /*!/flag:longrun*/  "Longrun power control") },
    { "lrti",     NC_("x86-flag", /*!/flag:lrti*/  "LongRun table interface") },
/* Other features, Linux-defined mapping */
    { "cxmmx",        NC_("x86-flag", /*!/flag:cxmmx*/  "Cyrix MMX extensions") },
    { "k6_mtrr",      NC_("x86-flag", /*!/flag:k6_mtrr*/  "AMD K6 nonstandard MTRRs") },
    { "cyrix_arr",    NC_("x86-flag", /*!/flag:cyrix_arr*/  "Cyrix ARRs (= MTRRs)") },
    { "centaur_mcr",  NC_("x86-flag", /*!/flag:centaur_mcr*/  "Centaur MCRs (= MTRRs)") },
    { "constant_tsc", NC_("x86-flag", /*!/flag:constant_tsc*/  "TSC ticks at a constant rate") },
    { "up",           NC_("x86-flag", /*!/flag:up*/  "SMP kernel running on UP") },
    { "art",          NC_("x86-flag", /*!/flag:art*/  "Always-Running Timer") },
    { "arch_perfmon", NC_("x86-flag", /*!/flag:arch_perfmon*/  "Intel Architectural PerfMon") },
    { "pebs",         NC_("x86-flag", /*!/flag:pebs*/  "Precise-Event Based Sampling") },
    { "bts",          NC_("x86-flag", /*!/flag:bts*/  "Branch Trace Store") },
    { "rep_good",     NC_("x86-flag", /*!/flag:rep_good*/  "rep microcode works well") },
    { "acc_power",    NC_("x86-flag", /*!/flag:acc_power*/  "AMD accumulated power mechanism") },
    { "nopl",         NC_("x86-flag", /*!/flag:nopl*/  "The NOPL (0F 1F) instructions") },
    { "xtopology",    NC_("x86-flag", /*!/flag:xtopology*/  "cpu topology enum extensions") },
    { "tsc_reliable", NC_("x86-flag", /*!/flag:tsc_reliable*/  "TSC is known to be reliable") },
    { "nonstop_tsc",  NC_("x86-flag", /*!/flag:nonstop_tsc*/  "TSC does not stop in C states") },
    { "cpuid",        NC_("x86-flag", /*!/flag:cpuid*/  "CPU has CPUID instruction itself") },
    { "extd_apicid",  NC_("x86-flag", /*!/flag:extd_apicid*/  "has extended APICID (8 bits)") },
    { "amd_dcm",      NC_("x86-flag", /*!/flag:amd_dcm*/  "multi-node processor") },
    { "aperfmperf",   NC_("x86-flag", /*!/flag:aperfmperf*/  "APERFMPERF") },
    { "eagerfpu",     NC_("x86-flag", /*!/flag:eagerfpu*/  "Non lazy FPU restore") },
    { "nonstop_tsc_s3", NC_("x86-flag", /*!/flag:nonstop_tsc_s3*/  "TSC doesn't stop in S3 state") },
    { "tsc_known_freq", NC_("x86-flag", /*!/flag:tsc_known_freq*/  "TSC has known frequency") },
    { "mce_recovery",   NC_("x86-flag", /*!/flag:mce_recovery*/  "CPU has recoverable machine checks") },
/* Intel-defined CPU features, CPUID level 0x00000001 (ecx)
 * See also Wikipedia and table 2-26 in Intel Advanced Vector Extensions Programming Reference */
    { "pni",       NC_("x86-flag", /*!/flag:pni*/  "SSE-3 (\"Prescott New Instructions\")") },
    { "pclmulqdq", NC_("x86-flag", /*!/flag:pclmulqdq*/  "Perform a Carry-Less Multiplication of Quadword instruction - accelerator for GCM)") },
    { "dtes64",    NC_("x86-flag", /*!/flag:dtes64*/  "64-bit Debug Store") },
    { "monitor",   NC_("x86-flag", /*!/flag:monitor*/  "Monitor/Mwait support (Intel SSE3 supplements)") },
    { "ds_cpl",    NC_("x86-flag", /*!/flag:ds_cpl*/  "CPL Qual. Debug Store") },
    { "vmx",       NC_("x86-flag", /*!/flag:vmx*/  "Hardware virtualization, Intel VMX") },
    { "smx",       NC_("x86-flag", /*!/flag:smx*/  "Safer mode TXT (TPM support)") },
    { "est",       NC_("x86-flag", /*!/flag:est*/  "Enhanced SpeedStep") },
    { "tm2",       NC_("x86-flag", /*!/flag:tm2*/  "Thermal Monitor 2") },
    { "ssse3",     NC_("x86-flag", /*!/flag:ssse3*/  "Supplemental SSE-3") },
    { "cid",       NC_("x86-flag", /*!/flag:cid*/  "Context ID") },
    { "sdbg",      NC_("x86-flag", /*!/flag:sdbg*/  "silicon debug") },
    { "fma",       NC_("x86-flag", /*!/flag:fma*/  "Fused multiply-add") },
    { "cx16",      NC_("x86-flag", /*!/flag:cx16*/  "CMPXCHG16B") },
    { "xtpr",      NC_("x86-flag", /*!/flag:xtpr*/  "Send Task Priority Messages") },
    { "pdcm",      NC_("x86-flag", /*!/flag:pdcm*/  "Performance Capabilities") },
    { "pcid",      NC_("x86-flag", /*!/flag:pcid*/  "Process Context Identifiers") },
    { "dca",       NC_("x86-flag", /*!/flag:dca*/  "Direct Cache Access") },
    { "sse4_1",    NC_("x86-flag", /*!/flag:sse4_1*/  "SSE-4.1") },
    { "sse4_2",    NC_("x86-flag", /*!/flag:sse4_2*/  "SSE-4.2") },
    { "x2apic",    NC_("x86-flag", /*!/flag:x2apic*/  "x2APIC") },
    { "movbe",     NC_("x86-flag", /*!/flag:movbe*/  "Move Data After Swapping Bytes instruction") },
    { "popcnt",    NC_("x86-flag", /*!/flag:popcnt*/  "Return the Count of Number of Bits Set to 1 instruction (Hamming weight, i.e. bit count)") },
    { "tsc_deadline_timer", NC_("x86-flag", /*/flag:tsc_deadline_timer*/  "Tsc deadline timer") },
    { "aes",         NC_("x86-flag", /*!/flag:aes*/  "Advanced Encryption Standard") },
    { "aes-ni",      NC_("x86-flag", /*!/flag:aes-ni*/  "Advanced Encryption Standard (New Instructions)") },
    { "xsave",       NC_("x86-flag", /*!/flag:xsave*/  "Save Processor Extended States: also provides XGETBY,XRSTOR,XSETBY") },
    { "avx",         NC_("x86-flag", /*!/flag:avx*/  "Advanced Vector Extensions") },
    { "f16c",        NC_("x86-flag", /*!/flag:f16c*/  "16-bit fp conversions (CVT16)") },
    { "rdrand",      NC_("x86-flag", /*!/flag:rdrand*/  "Read Random Number from hardware random number generator instruction") },
    { "hypervisor",  NC_("x86-flag", /*!/flag:hypervisor*/  "Running on a hypervisor") },
/* VIA/Cyrix/Centaur-defined CPU features, CPUID level 0xC0000001 */
    { "rng",     NC_("x86-flag", /*!/flag:rng*/  "Random Number Generator present (xstore)") },
    { "rng_en",  NC_("x86-flag", /*!/flag:rng_en*/  "Random Number Generator enabled") },
    { "ace",     NC_("x86-flag", /*!/flag:ace*/  "on-CPU crypto (xcrypt)") },
    { "ace_en",  NC_("x86-flag", /*!/flag:ace_en*/  "on-CPU crypto enabled") },
    { "ace2",    NC_("x86-flag", /*!/flag:ace2*/  "Advanced Cryptography Engine v2") },
    { "ace2_en", NC_("x86-flag", /*!/flag:ace2_en*/  "ACE v2 enabled") },
    { "phe",     NC_("x86-flag", /*!/flag:phe*/  "PadLock Hash Engine") },
    { "phe_en",  NC_("x86-flag", /*!/flag:phe_en*/  "PHE enabled") },
    { "pmm",     NC_("x86-flag", /*!/flag:pmm*/  "PadLock Montgomery Multiplier") },
    { "pmm_en",  NC_("x86-flag", /*!/flag:pmm_en*/  "PMM enabled") },
/* More extended AMD flags: CPUID level 0x80000001, ecx */
    { "lahf_lm",       NC_("x86-flag", /*!/flag:lahf_lm*/  "Load AH from Flags (LAHF) and Store AH into Flags (SAHF) in long mode") },
    { "cmp_legacy",    NC_("x86-flag", /*!/flag:cmp_legacy*/  "If yes HyperThreading not valid") },
    { "svm",           NC_("x86-flag", /*!/flag:svm*/  "\"Secure virtual machine\": AMD-V") },
    { "extapic",       NC_("x86-flag", /*!/flag:extapic*/  "Extended APIC space") },
    { "cr8_legacy",    NC_("x86-flag", /*!/flag:cr8_legacy*/  "CR8 in 32-bit mode") },
    { "abm",           NC_("x86-flag", /*!/flag:abm*/  "Advanced Bit Manipulation") },
    { "sse4a",         NC_("x86-flag", /*!/flag:sse4a*/  "SSE-4A") },
    { "misalignsse",   NC_("x86-flag", /*!/flag:misalignsse*/  "indicates if a general-protection exception (#GP) is generated when some legacy SSE instructions operate on unaligned data. Also depends on CR0 and Alignment Checking bit") },
    { "3dnowprefetch", NC_("x86-flag", /*!/flag:3dnowprefetch*/  "3DNow prefetch instructions") },
    { "osvw",          NC_("x86-flag", /*!/flag:osvw*/  "indicates OS Visible Workaround, which allows the OS to work around processor errata.") },
    { "ibs",           NC_("x86-flag", /*!/flag:ibs*/  "Instruction Based Sampling") },
    { "xop",           NC_("x86-flag", /*!/flag:xop*/  "extended AVX instructions") },
    { "skinit",        NC_("x86-flag", /*!/flag:skinit*/  "SKINIT/STGI instructions") },
    { "wdt",           NC_("x86-flag", /*!/flag:wdt*/  "Watchdog timer") },
    { "lwp",           NC_("x86-flag", /*!/flag:lwp*/  "Light Weight Profiling") },
    { "fma4",          NC_("x86-flag", /*!/flag:fma4*/  "4 operands MAC instructions") },
    { "tce",           NC_("x86-flag", /*!/flag:tce*/  "translation cache extension") },
    { "nodeid_msr",    NC_("x86-flag", /*!/flag:nodeid_msr*/  "NodeId MSR") },
    { "tbm",           NC_("x86-flag", /*!/flag:tbm*/  "Trailing Bit Manipulation") },
    { "topoext",       NC_("x86-flag", /*!/flag:topoext*/  "Topology Extensions CPUID leafs") },
    { "perfctr_core",  NC_("x86-flag", /*!/flag:perfctr_core*/  "Core Performance Counter Extensions") },
    { "perfctr_nb",    NC_("x86-flag", /*!/flag:perfctr_nb*/  "NB Performance Counter Extensions") },
    { "bpext",         NC_("x86-flag", /*!/flag:bpext*/  "data breakpoint extension") },
    { "ptsc",          NC_("x86-flag", /*!/flag:ptsc*/  "performance time-stamp counter") },
    { "perfctr_l2",    NC_("x86-flag", /*!/flag:perfctr_l2*/  "L2 Performance Counter Extensions") },
    { "mwaitx",        NC_("x86-flag", /*!/flag:mwaitx*/  "MWAIT extension (MONITORX/MWAITX)") },
/* Auxiliary flags: Linux defined - For features scattered in various CPUID levels */
    { "cpuid_fault",   NC_("x86-flag", /*!/flag:cpb*/  "Intel CPUID Faulting") },
    { "cpb",           NC_("x86-flag", /*!/flag:cpb*/  "AMD Core Performance Boost") },
    { "epb",           NC_("x86-flag", /*!/flag:epb*/  "IA32_ENERGY_PERF_BIAS support") },
    { "hw_pstate",     NC_("x86-flag", /*!/flag:hw_pstate*/  "AMD HW-PState") },
    { "proc_feedback", NC_("x86-flag", /*!/flag:proc_feedback*/  "AMD ProcFeedbackInterface") },
    { "intel_pt",      NC_("x86-flag", /*!/flag:intel_pt*/  "Intel Processor Tracing") },
    { "pti",           NC_("x86-flag", /*!/flag:pti*/  "Kernel PAge Table Isolation Enabled") },
    { "ibrs",          NC_("x86-flag", /*!/flag:ibrs*/  "Set/clear IBRS on kernel entry/exit") },
    { "ssbd",          NC_("x86-flag", /*!/flag:ssbd*/  "Speculative Store Bypass Disable") },
    { "ibrs",          NC_("x86-flag", /*!/flag:ibrs*/  "Indirect Branch Restricted Speculation") },
    { "ibpb",          NC_("x86-flag", /*!/flag:ibpb*/  "Indirect Branch Prediction Barrier without guaranteed RSB flush") },
    { "stibp",         NC_("x86-flag", /*!/flag:stibp*/  "Single Thread Indirect Branch Predictors") },
    { "ibrs_enhanced", NC_("x86-flag", /*!/flag:ibrs_enhanced*/  "Enhanced IBRS") },
/* Virtualization flags: Linux defined */
    { "tpr_shadow",   NC_("x86-flag", /*!/flag:tpr_shadow*/  "Intel TPR Shadow") },
    { "vnmi",         NC_("x86-flag", /*!/flag:vnmi*/  "Intel Virtual NMI") },
    { "flexpriority", NC_("x86-flag", /*!/flag:flexpriority*/  "Intel FlexPriority") },
    { "ept",          NC_("x86-flag", /*!/flag:ept*/  "Intel Extended Page Table") },
    { "vpid",         NC_("x86-flag", /*!/flag:vpid*/  "Intel Virtual Processor ID") },
    { "vmmcall",      NC_("x86-flag", /*!/flag:vmmcall*/  "prefer VMMCALL to VMCALL") },
    { "ept_ad",       NC_("x86-flag", /*!/flag:ept_ad*/  "Intel Extended Page Table access-dirty bit") },
/* AMD-defined CPU features, CPUID level 0x00000007:1 (eax), word 12 */
    { "avx512_bf16",  NC_("x86-flag", /*!/flag:avx512_bf16*/  "AVX512 BFLOAT16 instructions") },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ebx) */
    { "fsgsbase",   NC_("x86-flag", /*!/flag:fsgsbase*/  "{RD/WR}{FS/GS}BASE instructions") },
    { "tsc_adjust", NC_("x86-flag", /*!/flag:tsc_adjust*/  "TSC adjustment MSR") },
    { "sgx",        NC_("x86-flag", /*!/flag:sgx*/  "Software Guard Extensions") },
    { "bmi1",       NC_("x86-flag", /*!/flag:bmi1*/  "1st group bit manipulation extensions") },
    { "hle",        NC_("x86-flag", /*!/flag:hle*/  "Hardware Lock Elision") },
    { "avx2",       NC_("x86-flag", /*!/flag:avx2*/  "AVX2 instructions") },
    { "smep",       NC_("x86-flag", /*!/flag:smep*/  "Supervisor Mode Execution Protection") },
    { "bmi2",       NC_("x86-flag", /*!/flag:bmi2*/  "2nd group bit manipulation extensions") },
    { "erms",       NC_("x86-flag", /*!/flag:erms*/  "Enhanced REP MOVSB/STOSB") },
    { "invpcid",    NC_("x86-flag", /*!/flag:invpcid*/  "Invalidate Processor Context ID") },
    { "rtm",        NC_("x86-flag", /*!/flag:rtm*/  "Restricted Transactional Memory") },
    { "cqm",        NC_("x86-flag", /*!/flag:cqm*/  "Cache QoS Monitoring") },
    { "mpx",        NC_("x86-flag", /*!/flag:mpx*/  "Memory Protection Extension") },
    { "avx512f",    NC_("x86-flag", /*!/flag:avx512f*/  "AVX-512 foundation") },
    { "avx512dq",   NC_("x86-flag", /*!/flag:avx512dq*/  "AVX-512 Double/Quad instructions") },
    { "rdseed",     NC_("x86-flag", /*!/flag:rdseed*/  "The RDSEED instruction") },
    { "adx",        NC_("x86-flag", /*!/flag:adx*/  "The ADCX and ADOX instructions") },
    { "smap",       NC_("x86-flag", /*!/flag:smap*/  "Supervisor Mode Access Prevention") },
    { "avx512ifma", NC_("x86-flag", /*!/flag:avx512ifma*/  "AVX-512 Integer Fused Multiply-Add instructions") },
    { "clflushopt", NC_("x86-flag", /*!/flag:clflushopt*/  "CLFLUSHOPT instruction") },
    { "clwb",       NC_("x86-flag", /*!/flag:clwb*/  "CLWB instruction") },
    { "avx512pf",   NC_("x86-flag", /*!/flag:avx512pf*/  "AVX-512 Prefetch") },
    { "avx512er",   NC_("x86-flag", /*!/flag:avx512er*/  "AVX-512 Exponential and Reciprocal") },
    { "avx512cd",   NC_("x86-flag", /*!/flag:avx512cd*/  "AVX-512 Conflict Detection") },
    { "sha_ni",     NC_("x86-flag", /*!/flag:sha_ni*/  "SHA1/SHA256 Instruction Extensions") },
    { "avx512bw",   NC_("x86-flag", /*!/flag:avx512bw*/  "AVX-512 Byte/Word instructions") },
    { "avx512vl",   NC_("x86-flag", /*!/flag:avx512vl*/  "AVX-512 128/256 Vector Length extensions") },
    { "md_clear",   NC_("x86-flag", /*!/flag:md_clear*/  "VERW clears CPU buffers") },
    { "flush_l1d",  NC_("x86-flag", /*!/flag:flush_l1d*/  "Flush L1D cache") },
    { "arch_capabilities",  NC_("x86-flag", /*!/flag:arch_capabilities*/  "IA32_ARCH_CAPABILITIES MSR (Intel)") },
/* Extended state features, CPUID level 0x0000000d:1 (eax) */
    { "xsaveopt",   NC_("x86-flag", /*!/flag:xsaveopt*/  "Optimized XSAVE") },
    { "xsavec",     NC_("x86-flag", /*!/flag:xsavec*/  "XSAVEC") },
    { "xgetbv1",    NC_("x86-flag", /*!/flag:xgetbv1*/  "XGETBV with ECX = 1") },
    { "xsaves",     NC_("x86-flag", /*!/flag:xsaves*/  "XSAVES/XRSTORS") },
/* Extended auxillary flags: Linux defined - fro features scatterd in various CPUID levels */
    { "cqm_llc",        NC_("x86-flag", /*!/flag:cqm_llc*/  "LLC QoS") },
    { "cqm_occup_llc",  NC_("x86-flag", /*!/flag:cqm_occup_llc*/  "LLC occupancy monitoring") },
    { "cqm_mbm_total",  NC_("x86-flag", /*!/flag:cqm_mbm_total*/  "LLC total MBM monitoring") },
    { "cqm_mbm_local",  NC_("x86-flag", /*!/flag:cqm_mbm_local*/  "LLC local MBM monitoring") },
    { "split_lock_detect", NC_("x86-flag", /*!/flag:split_lock_detect*/  "AC for split lock") },
/* AMD-defined CPU features, CPUID level 0x80000008 (ebx) */
    { "clzero",         NC_("x86-flag", /*!/flag:clzero*/  "CLZERO instruction") },
    { "irperf",         NC_("x86-flag", /*!/flag:irperf*/  "instructions retired performance counter") },
    { "xsaveerptr",     NC_("x86-flag", /*!/flag:xsaveerptr*/  "Always save/restore FP error pointers") },
/* Thermal and Power Management leaf, CPUID level 0x00000006 (eax) */
    { "dtherm",         NC_("x86-flag", /*!/flag:dtherm*/  "digital thermal sensor") }, /* formerly dts */
    { "ida",            NC_("x86-flag", /*!/flag:ida*/  "Intel Dynamic Acceleration") },
    { "arat",           NC_("x86-flag", /*!/flag:arat*/  "Always Running APIC Timer") },
    { "pln",            NC_("x86-flag", /*!/flag:pln*/  "Intel Power Limit Notification") },
    { "pts",            NC_("x86-flag", /*!/flag:pts*/  "Intel Package Thermal Status") },
    { "hwp",            NC_("x86-flag", /*!/flag:hwp*/  "Intel Hardware P-states") },
    { "hwp_notify",     NC_("x86-flag", /*!/flag:hwp_notify*/  "HWP notification") },
    { "hwp_act_window", NC_("x86-flag", /*!/flag:hwp_act_window*/  "HWP Activity Window") },
    { "hwp_epp",        NC_("x86-flag", /*!/flag:hwp_epp*/  "HWP Energy Performance Preference") },
    { "hwp_pkg_req",    NC_("x86-flag", /*!/flag:hwp_pkg_req*/  "HWP package-level request") },
/* AMD SVM Feature Identification, CPUID level 0x8000000a (edx) */
    { "npt",            NC_("x86-flag", /*!/flag:npt*/  "AMD Nested Page Table support") },
    { "lbrv",           NC_("x86-flag", /*!/flag:lbrv*/  "AMD LBR Virtualization support") },
    { "svm_lock",       NC_("x86-flag", /*!/flag:svm_lock*/  "AMD SVM locking MSR") },
    { "nrip_save",      NC_("x86-flag", /*!/flag:nrip_save*/  "AMD SVM next_rip save") },
    { "tsc_scale",      NC_("x86-flag", /*!/flag:tsc_scale*/  "AMD TSC scaling support") },
    { "vmcb_clean",     NC_("x86-flag", /*!/flag:vmcb_clean*/  "AMD VMCB clean bits support") },
    { "flushbyasid",    NC_("x86-flag", /*!/flag:flushbyasid*/  "AMD flush-by-ASID support") },
    { "decodeassists",  NC_("x86-flag", /*!/flag:decodeassists*/  "AMD Decode Assists support") },
    { "pausefilter",    NC_("x86-flag", /*!/flag:pausefilter*/  "AMD filtered pause intercept") },
    { "pfthreshold",    NC_("x86-flag", /*!/flag:pfthreshold*/  "AMD pause filter threshold") },
    { "avic",           NC_("x86-flag", /*!/flag:avic*/  "Virtual Interrupt Controller") },
    { "v_vmsave_vmload",NC_("x86-flag", /*!/flag:v_vmsave_vmload*/  "Virtual VMSAVE VMLOAD") },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (ecx) */
    { "avx512vbmi",   NC_("x86-flag", /*!/flag:avx512vbmi*/  "AVX512 Vector Bit Manipulation instructions") },
    { "umip",           NC_("x86-flag", /*!/flag:umip*/  "User Mode Instruction Protection") },
    { "pku",            NC_("x86-flag", /*!/flag:pku*/  "Protection Keys for Userspace") },
    { "ospke",          NC_("x86-flag", /*!/flag:ospke*/  "OS Protection Keys Enable") },
    { "avx512_vbmi2",   NC_("x86-flag", /*!/flag:avx512_vbmi2*/  "Additional AVX512 Vector Bit Manipulation instructions") },
    { "gfni",           NC_("x86-flag", /*!/flag:gfni*/  "Galois Field New instructions") },
    { "vaes",           NC_("x86-flag", /*!/flag:vaes*/  "Vector AES") },
    { "vpclmulqdq",     NC_("x86-flag", /*!/flag:vpclmulqdq*/  "Carry-Less Multiplication Double QuadWord") },
    { "avx512_vnni",    NC_("x86-flag", /*!/flag:avx512_vnni*/  "Vector Neural Network Instructions") },
    { "avx512_bitalg",  NC_("x86-flag", /*!/flag:avx512_bitalg*/  "Support for VPOPCNT[B,W] and VPSHUF-BITQMB instructions") },
    { "avx512_vpopcntdq", NC_("x86-flag", /*!/flag:avx512_vpopcntdq*/  "POPCNT for vectors of DW/DQ") },
    { "rdpid",          NC_("x86-flag", /*!/flag:rdpid*/  "RDPID instruction") },
    { "sgx_lc",         NC_("x86-flag", /*!/flag:sgx_lc*/  "Software Guard Extensions Launch Control") },
/* AMD-defined CPU features, CPUID level 0x80000007 (ebx) */
    { "overflow_recov", NC_("x86-flag", /*!/flag:overflow_recov*/  "MCA overflow recovery support") },
    { "succor",         NC_("x86-flag", /*!/flag:succor*/  "uncorrectable error containment and recovery") },
    { "smca",           NC_("x86-flag", /*!/flag:smca*/  "Scalable MCA") },
/* Intel-defined CPU features, CPUID level 0x00000007:0 (edx), word 18 */
    { "fsrm",           NC_("x86-flag", /*!/flag:fsrm*/  "Fast Short Rep Mov") },

/* bug workarounds */
    { "bug:f00f",       NC_("x86-flag", /*!/bug:f00f*/  "Intel F00F bug")    },
    { "bug:fdiv",       NC_("x86-flag", /*!/bug:fdiv*/  "FPU FDIV")          },
    { "bug:coma",       NC_("x86-flag", /*!/bug:coma*/  "Cyrix 6x86 coma")   },
    { "bug:tlb_mmatch", NC_("x86-flag", /*!/bug:tlb_mmatch*/  "AMD Erratum 383")   },
    { "bug:apic_c1e",   NC_("x86-flag", /*!/bug:apic_c1e*/  "AMD Erratum 400")   },
    { "bug:11ap",       NC_("x86-flag", /*!/bug:11ap*/  "Bad local APIC aka 11AP")  },
    { "bug:fxsave_leak",      NC_("x86-flag", /*!/bug:fxsave_leak*/  "FXSAVE leaks FOP/FIP/FOP") },
    { "bug:clflush_monitor",  NC_("x86-flag", /*!/bug:clflush_monitor*/  "AAI65, CLFLUSH required before MONITOR") },
    { "bug:sysret_ss_attrs",  NC_("x86-flag", /*!/bug:sysret_ss_attrs*/  "SYSRET doesn't fix up SS attrs") },
    { "bug:espfix",       NC_("x86-flag", /*!/bug:espfix*/  "IRET to 16-bit SS corrupts ESP/RSP high bits") },
    { "bug:null_seg",     NC_("x86-flag", /*!/bug:null_seg*/  "Nulling a selector preserves the base") },         /* see: detect_null_seg_behavior() */
    { "bug:swapgs_fence", NC_("x86-flag", /*!/bug:swapgs_fence*/  "SWAPGS without input dep on GS") },
    { "bug:monitor",      NC_("x86-flag", /*!/bug:monitor*/  "IPI required to wake up remote CPU") },
    { "bug:amd_e400",     NC_("x86-flag", /*!/bug:amd_e400*/  "AMD Erratum 400") },
    { "bug:cpu_insecure",     NC_("x86-flag", /*!/bug:cpu_insecure & bug:cpu_meltdown*/  "CPU is affected by meltdown attack and needs kernel page table isolation") },
    { "bug:cpu_meltdown",     NC_("x86-flag", /*!/bug:cpu_insecure & bug:cpu_meltdown*/  "CPU is affected by meltdown attack and needs kernel page table isolation") },
    { "bug:spectre_v1",     NC_("x86-flag", /*!/bug:spectre_v1*/  "CPU is affected by Spectre variant 1 attack with conditional branches") },
    { "bug:spectre_v2",     NC_("x86-flag", /*!/bug:spectre_v2*/  "CPU is affected by Spectre variant 2 attack with indirect branches") },
    { "bug:spec_store_bypass", NC_("x86-flag", /*!/bug:spec_store_bypass*/  "CPU is affected by speculative store bypass attack") },
    { "bug:l1tf",           NC_("x86-flag", /*!/bug:l1tf*/  "CPU is affected by L1 Terminal Fault") },
    { "bug:mds",            NC_("x86-flag", /*!/bug:mds*/  "CPU is affected by Microarchitectual data sampling") },
    { "bug:swapgs",         NC_("x86-flag", /*!/bug:swapgs*/  "CPU is affected by speculation through SWAPGS") },
    { "bug:itlb_multihit",  NC_("x86-flag", /*!/bug:itlb_multihit*/  "CPU may incur MCE during certain page attribute changes") },
    { "bug:srbds",          NC_("x86-flag", /*!/bug:srbds*/  "CPU may leak RNG bits if not mitigated") },
    { "bug:mmio_stale_data",NC_("x86-flag", /*!/bug:mmio_stale_data*/  "CPU is affected by Proccesor MMIO Stale Data vulnerbilities") },
    { "bug:mmio_unknown",   NC_("x86-flag", /*!/bug:mmio_unknown*/  "MMIO State Data status is unknown") },
    { "bug:retbleed",       NC_("x86-flag", /*!/bug:retbleed*/  "CPU is affected by RETBleed") },
    { "bug:eibrs_pbrsb",    NC_("x86-flag", /*!/bug:eibrs_pbrsb*/  "EIBRS is vulnerable to Post Barrier Address Predictions") },
    { "bug:gds",            NC_("x86-flag", /*!/bug:gds*/  "CPU is affected by Gather Data Sampling") },
/* bug workarounds 2 */
    { "bug:srso",           NC_("x86-flag", /*!/bug:srso*/  "AMD BTB untrain RETs") },
    { "bug:bhi",            NC_("x86-flag", /*!/bug:bhi*/  "CPU is affected by Branch History Injection") },
    { "bug:spectre_v2_user",NC_("x86-flag", /*!/bug:spectre_v2_user*/  "CPU is affected by Spectre variant 2 attack between user processes") },
    { "bug:its",            NC_("x86-flag", /*!/bug:its*/  "CPU is affected by Indirect Target Selection") },
    { "bug:its_native_only",NC_("x86-flag", /*!/bug:its_native_only*/  "CPU is affected by ITS, VMX is not affected") },
    { "bug:vmscape",        NC_("x86-flag", /*!/bug:vmscape*/  "CPU is affected by VMSCAPE attacks from guests") },
/* power management
 * ... from arch/x86/kernel/cpu/powerflags.h */
    { "pm:ts",            NC_("x86-flag", /*!/flag:pm:ts*/  "temperature sensor")     },
    { "pm:fid",           NC_("x86-flag", /*!/flag:pm:fid*/  "frequency id control")   },
    { "pm:vid",           NC_("x86-flag", /*!/flag:pm:vid*/  "voltage id control")     },
    { "pm:ttp",           NC_("x86-flag", /*!/flag:pm:ttp*/  "thermal trip")           },
    { "pm:tm",            NC_("x86-flag", /*!/flag:pm:tm*/  "hardware thermal control")   },
    { "pm:stc",           NC_("x86-flag", /*!/flag:pm:stc*/  "software thermal control")   },
    { "pm:100mhzsteps",   NC_("x86-flag", /*!/flag:pm:100mhzsteps*/  "100 MHz multiplier control") },
    { "pm:hwpstate",      NC_("x86-flag", /*!/flag:pm:hwpstate*/  "hardware P-state control")   },
    { "pm:cpb",           NC_("x86-flag", /*!/flag:pm:cpb*/  "core performance boost")     },
    { "pm:eff_freq_ro",   NC_("x86-flag", /*!/flag:pm:eff_freq_ro*/  "Readonly aperf/mperf")       },
    { "pm:proc_feedback", NC_("x86-flag", /*!/flag:pm:proc_feedback*/  "processor feedback interface") },
    { "pm:acc_power",     NC_("x86-flag", /*!/flag:pm:acc_power*/  "accumulated power mechanism")  },
    { NULL, NULL},
};

static struct flag_to_meaning *tab_flag_meaning;

//static char all_flags[4096] = "";

#if JSON_CHECK_VERSION(0,20,0)
static void build_meaning_table_iter(JsonObject *object,
                                     const gchar *member_name,
                                     JsonNode *member_node,
                                     gpointer user_data)
{
    int *i = user_data;

    tab_flag_meaning[*i] = (struct flag_to_meaning) {
        .name = g_strdup(member_name),
        .meaning = g_strdup(json_node_get_string(member_node)),
    };

    (*i)++;
}
#endif

void cpuflags_x86_init(void)
{
    gchar *flag_json = g_build_filename(g_get_user_config_dir(), "hardinfo2",
                                        "cpuflags.json", NULL);
    gboolean use_builtin_table = TRUE;
#if JSON_CHECK_VERSION(0,20,0)
    if (!g_file_test(flag_json, G_FILE_TEST_EXISTS))
        goto use_builtin_table;

    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_file(parser, flag_json, NULL))
        goto use_builtin_table_with_json;

    JsonNode *root = json_parser_get_root(parser);
    if (json_node_get_node_type(root) != JSON_NODE_OBJECT)
        goto use_builtin_table_with_json;

    JsonObject *x86_flags =
        json_object_get_object_member(json_node_get_object(root), "x86");
    if (!x86_flags)
        goto use_builtin_table_with_json;

    tab_flag_meaning =
        g_new(struct flag_to_meaning, json_object_get_size(x86_flags) + 1);
    int i = 0;
    json_object_foreach_member(x86_flags, build_meaning_table_iter, &i);
    tab_flag_meaning[i] = (struct flag_to_meaning){NULL, NULL};
    use_builtin_table = FALSE;

use_builtin_table_with_json:
    g_object_unref(parser);
use_builtin_table:
#endif
    g_free(flag_json);

    if (use_builtin_table)
        tab_flag_meaning = (struct flag_to_meaning *)builtin_tab_flag_meaning;
}

const char *x86_flag_meaning(const char *flag) {
    int i;

    if (!flag)
        return NULL;

    for (i = 0; tab_flag_meaning[i].name; i++) {
        if (strcmp(tab_flag_meaning[i].name, flag) == 0) {
            if (tab_flag_meaning[i].meaning != NULL)
                return C_("x86-flag", tab_flag_meaning[i].meaning);
            else return NULL;
        }
    }

    return NULL;
}

