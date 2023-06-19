// EPOS RISC-V Interrupt Controller Initialization

#include <architecture/cpu.h>
#include <machine/ic.h>
#include <machine/timer.h>

__BEGIN_SYS

// Class methods
void IC::init()
{
    db<Init, IC>(TRC) << "IC::init()" << endl;

    assert(CPU::int_disabled()); // will be reenabled at Thread::init() by Context::load()

    disable(); // will be enabled on demand as handlers are registered

    // Set all exception handlers to exception()
    for(Interrupt_Id i = 0; i < EXCS; i++)
        _int_vector[i] = &exception;

    // Install the syscall trap handler
#ifdef __kernel__
    _int_vector[INT_SYSCALL] = &CPU::syscalled;
#endif

    // Set all interrupt handlers to int_not()
    for(Interrupt_Id i = EXCS; i < INTS; i++)
        _int_vector[i] = &int_not;
}

__END_SYS
