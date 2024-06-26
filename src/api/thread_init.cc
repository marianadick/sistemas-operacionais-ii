// EPOS Thread Initialization

#include <machine/timer.h>
#include <machine/ic.h>
#include <system.h>
#include <process.h>

__BEGIN_SYS

extern "C" { void __epos_app_entry(); }

void Thread::init()
{
    db<Init, Thread>(TRC) << "Thread::init()" << endl;

    Criterion::init();

#ifdef __library__

    typedef int (Main)();

    // If EPOS is a library, then adjust the application entry point to __epos_app_entry, which will directly call main().
    // In this case, _init will have already been called, before Init_Application to construct MAIN's global objects.
    Main * main = reinterpret_cast<Main *>(__epos_app_entry);

    new (SYSTEM) Thread(Thread::Configuration(Thread::RUNNING, Thread::MAIN), main);

#else

    typedef int (Main)(int argc, char * argv[]);

    System_Info * si = System::info();

    Address_Space * as = new (SYSTEM) Address_Space(MMU::current());
    Segment * cs = new (SYSTEM) Segment(Log_Addr(si->pmm.app_code_pt), 0, MMU::pages(si->lm.app_code_size), Segment::Flags::APPC);
    Segment * ds = new (SYSTEM) Segment(Log_Addr(si->pmm.app_data_pt), 0, MMU::pages(si->lm.app_data_size), Segment::Flags::APPD);
    Log_Addr code = si->lm.app_code;
    Log_Addr data = si->lm.app_data;
    Main * main = reinterpret_cast<Main *>(si->lm.app_entry);
    int argc = static_cast<int>(si->lm.app_extra_size);
    char ** argv = reinterpret_cast<char **>(si->lm.app_extra);

    new (SYSTEM) Task(as, cs, ds, code, data, main, argc, argv);

    if(si->lm.has_ext)
        db<Init>(INF) << "Thread::init: additional data from mkbi at " << reinterpret_cast<void *>(si->lm.app_extra) << ":" << si->lm.app_extra_size << endl;

#endif

    // Idle thread creation does not cause rescheduling (see Thread::constructor_epilogue)
    new (SYSTEM) Thread(Thread::Configuration(Thread::READY, Thread::IDLE), &Thread::idle);

    // The installation of the scheduler timer handler does not need to be done after the
    // creation of threads, since the constructor won't call reschedule() which won't call
    // dispatch that could call timer->reset()
    // Letting reschedule() happen during thread creation is also harmless, since MAIN is
    // created first and dispatch won't replace it nor by itself neither by IDLE (which
    // has a lower priority)
    if(Criterion::timed)
        _timer = new (SYSTEM) Scheduler_Timer(QUANTUM, time_slicer);

    // No more interrupts until we reach init_end
    CPU::int_disable();

    // Transition from CPU-based locking to thread-based locking
    This_Thread::not_booting();
}

__END_SYS
