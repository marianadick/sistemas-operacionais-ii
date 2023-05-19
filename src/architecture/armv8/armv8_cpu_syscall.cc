// EPOS ARMv8 System Call Function Implementation

#include <architecture/armv8/armv8_cpu.h>
#include <machine/ic.h>

__BEGIN_SYS

void CPU::syscall(void * message)
{
    CPU::r1(reinterpret_cast<CPU::Reg>(message));
    CPU::svc();
}

__END_SYS
