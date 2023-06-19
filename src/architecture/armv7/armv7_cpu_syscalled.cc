// EPOS ARMv7 System Call Entry Implementation

#include <architecture/armv7/armv7_cpu.h>

#ifndef __library__

extern "C" { void _exec(void *); }
extern "C" { void _software_interrupt() __attribute__ ((alias("_ZN4EPOS1S3CPU9syscalledEv"))); }

__BEGIN_SYS

void CPU::syscalled()
{
#ifdef __cortex_a53__

    // We get here when an APP triggers INT_SYSCALL (through svc) with the message address in r1.
    // This is run in SVC mode.

    ASM("       stmfd   sp!, {r0-r3, r12, lr}           \n"     // save current context (lr, sp and spsr are banked registers)
        "       mrs     r0, spsr                        \n"
        "       push    {r0}                            \n");
    _exec(reinterpret_cast<void *>(CPU::r1()));                 // the message to EPOS Framework is passed on register r1
    ASM("       pop     {r0}                            \n"     // pop saved SPSR
        "       msr     spsr_cfxs, r0                   \n"
        "       ldmfd   sp!, {r0-r3, r12, pc}^          \n");   // restore the context (including PC in ldmfd cause a mode change to the mode before the interrupt)

#endif
}

void ARMv7_A::Context::first_dispatch()
{
    pop(false, false);
}

__END_SYS

#endif
