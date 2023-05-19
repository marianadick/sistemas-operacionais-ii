// EPOS IA32 CPU System Call Entry Implementation

#include <architecture/ia32/ia32_cpu.h>
#include <architecture/ia32/ia32_mmu.h>
#include <machine/pc/pc_ic.h>
#include <system/memory_map.h>

__BEGIN_SYS

#ifdef __kernel__

void CPU::syscalled()
{
    // We get here when an APP triggers INT_SYSCALL with the message address in CX.
    // This is always a cross-level call (from L3 to L0), since the PL bits in CS do change on the interrupt gate.
    // The CPU saves the user-level stack pointer (USP) in the stack and restores the system-level stack pointer (SP0) from the TSS.
    // Stack contents at this point are always: SS, USP, FLAGS, CS, IP.
    // CX holds the pointer to the message.

    Context::push(true);
    ASM("call _exec"); // message is already in CX and _exec has attribute "thiscall", which expects the parameter on CX
    Context::pop(true);
}

#endif

#ifndef __library__

void CPU::Context::first_dispatch()
{
    Context::pop(false, true);
};

#endif

__END_SYS

