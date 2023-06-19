// EPOS RISC-V 32 System Call Entry Implementation

#include <architecture/rv32/rv32_cpu.h>

extern "C" { void _exec(void *); }

__BEGIN_SYS

#ifdef __kernel__

void CPU::syscalled(unsigned int int_id)
{
    // We get here when an APP triggers INT_SYSCALL (i.e. ecall)
    if(Traits<Build>::MODE == Traits<Build>::KERNEL) {
        _exec(reinterpret_cast<void *>(CPU::a1())); 	// the message to EPOS Framework is passed on register a1
        CPU::fr(4);                    			// tell IC::entry to perform PC = PC + 4 on return
    }
}

#endif

#ifndef __library__

void CPU::Context::first_dispatch()
{
    CPU::Context::pop(true);
    CPU::iret();
};

#endif


__END_SYS
