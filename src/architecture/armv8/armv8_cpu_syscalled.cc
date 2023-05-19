// EPOS ARMv8 System Call Entry Implementation

#include <architecture/armv8/armv8_cpu.h>

extern "C" { void _exec(void *); }
extern "C" { void _software_interrupt() __attribute__ ((alias("_ZN4EPOS1S3CPU9syscalledEv"))); }

__BEGIN_SYS

void CPU::syscalled()
{
    // We get here when an APP triggers INT_SYSCALL (through svc) with the message address in x1
    // This is run inside SVC mode

//    ASM("       stmfd   sp!, {r0-r3, r12, lr}           \n"     // save current context (lr, sp and spsr are banked registers)
//        "       mrs     r0, spsr                        \n"
//        "       push    {r0}                            \n");
    if(Traits<Build>::MODE == Traits<Build>::KERNEL)
        _exec(reinterpret_cast<void *>(CPU::r1()));             // the message to EPOS Framework is passed on register r1
//    ASM("       pop     {r0}                            \n"     // pop saved SPSR
//        "       msr     spsr_cfxs, r0                   \n"
//        "       ldmfd   sp!, {r0-r3, r12, pc}^          \n");   // restore the context (including PC in ldmfd cause a mode change to the mode before the interrupt)
}

void CPU::context_load_helper()
{
//    ASM("       pop     {r12}                           \n"
//        "       msr     sp_usr, r12                     \n"
//        "       pop     {r12}                           \n"
//        "       msr     lr_usr, r12                     \n"
//        "       pop     {r12}                           \n"
//        "       msr     spsr_cfxs, r12                  \n"
//        "       ldmfd   sp!, {r0-r12, lr, pc}^          \n");
}

__END_SYS
