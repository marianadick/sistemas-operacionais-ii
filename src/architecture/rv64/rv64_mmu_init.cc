#include <architecture/mmu.h>
#include <system.h>

extern "C" char _edata;
extern "C" char __bss_start;
extern "C" char _end;

__BEGIN_SYS

void Sv39_MMU::init()
{
    /* OLD CONFIGURATION */
    // db<Init, MMU>(TRC) << "MMU::init()" << endl;
    // free(System::info()->pmm.free1_base, pages(System::info()->pmm.free1_top - System::info()->pmm.free1_base));

    db<Init, MMU>(TRC) << "Sv39_MMU::init()" << endl;
    db<Init, MMU>(INF) << "Sv39_MMU::init::edata=" << &_edata << ",bss_start=" << &__bss_start << ",bss_end=" << &_end << endl;

    // free no page_table
    free(Memory_Map::RAM_BASE, pages(Memory_Map::RAM_BASE + (512 * 0x200000) - Memory_Map::RAM_BASE));

    // free na stack
    free(Memory_Map::RAM_TOP + 1 - Traits<Machine>::STACK_SIZE, pages(Traits<Machine>::STACK_SIZE));
}

__END_SYS