// EPOS ARMv8 MMU Mediator Initialization

#include <architecture/mmu.h>
#include <system.h>

#ifdef __cortex_a53__

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void ARMv8_MMU::init()
{
    db<Init, MMU>(TRC) << "MMU::init()" << endl;

    db<Init, MMU>(INF) << "MMU::init::dat.e=" << reinterpret_cast<void *>(&_edata) << ",bss.b=" << reinterpret_cast<void *>(&__bss_start) << ",bss.e=" << reinterpret_cast<void *>(&_end) << endl;

    // For machines that do not feature a real MMU, frame size = 1 byte
    // Allocations (using Grouping_List<Frame>::search_decrementing() start from the end
    free(&_end, pages(Memory_Map::FREE_TOP - reinterpret_cast<unsigned long>(&_end)));
   
    // Remember the master page directory (created during SETUP)
    _master = current();
    db<Init, MMU>(INF) << "MMU::master page directory=" << _master << endl;
}

__END_SYS

#endif
