#include <architecture/mmu.h>
#include <system.h>

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void MMU::init()
{
    /* OLD CONFIGURATION */
    // db<Init, MMU>(TRC) << "MMU::init()" << endl;
    // free(System::info()->pmm.free1_base, pages(System::info()->pmm.free1_top - System::info()->pmm.free1_base));

    db<Init, MMU>(TRC) << "MMU::init()" << endl;
    db<Init, MMU>(INF) << "MMU::init::edata=" << &_edata << ",bss_start=" << &__bss_start << ",_end=" << &_end << endl;
    free(System::info()->pmm.free1_base, pages(System::info()->pmm.free1_top - System::info()->pmm.free1_base));
}

__END_SYS