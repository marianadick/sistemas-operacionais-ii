// EPOS SiFive-U (RISC-V) SETUP

#include <architecture.h>
#include <machine.h>
#include <utility/elf.h>
#include <utility/string.h>

using namespace EPOS::S;
typedef unsigned long Reg;

// timer handler
extern "C" [[gnu::interrupt, gnu::aligned(8)]] void _mmode_forward()
{
    // Retrieve interrupt ID
    Reg id = CPU::mcause();
    if ((id & CLINT::INT_MASK) == CLINT::IRQ_MAC_TIMER) {
        Timer::reset();
        CPU::sie(CPU::STI); // Enable interrupts
    }
    Reg interrupt_id = 1 << ((id & CLINT::INT_MASK) - 2);
    // Check if interrupts are enabled and the interrupt is enabled in the SIE register
    if (CPU::int_enabled() && (CPU::sie() & (interrupt_id)))
        CPU::mip(interrupt_id); // setting the bit to indicate that the interruption was handled
}

extern "C"
{
    void _start();

    void _int_entry();

    // SETUP entry point is in .init (and not in .text), so it will be linked first and will be the first function after the ELF header in the image
    void _entry() __attribute__((used, naked, section(".init")));
    void _setup();

    // LD eliminates this variable while performing garbage collection, that's why the used attribute.
    char __boot_time_system_info[sizeof(EPOS::S::System_Info)] __attribute__((used)) = "<System_Info placeholder>"; // actual System_Info will be added by mkbi!
}

__BEGIN_SYS

extern OStream kout, kerr;

class Setup
{
private:
    // Physical memory map
    static const unsigned long RAM_BASE  = Memory_Map::RAM_BASE;
    static const unsigned long RAM_TOP   = Memory_Map::RAM_TOP;
    static const unsigned long APP_LOW   = Memory_Map::APP_LOW;
    static const unsigned long APP_HIGH  = Memory_Map::APP_HIGH;
    static const unsigned long MIO_BASE  = Memory_Map::MIO_BASE;
    static const unsigned long MIO_TOP   = Memory_Map::MIO_TOP;
    static const unsigned long FREE_BASE = Memory_Map::FREE_BASE;
    static const unsigned long FREE_TOP  = Memory_Map::FREE_TOP;
    static const unsigned long SETUP     = Memory_Map::SETUP;
    static const unsigned long BOOT_STACK  = Memory_Map::BOOT_STACK;
    static const unsigned long PAGE_TABLES = Memory_Map::PAGE_TABLES;

    static const unsigned int PT_ENTRIES = MMU::PT_ENTRIES;
    static const unsigned int PD_ENTRIES = PT_ENTRIES;

    // Architecture Imports
    typedef CPU::Reg Reg;
    typedef CPU::Phy_Addr Phy_Addr;
    typedef CPU::Log_Addr Log_Addr;
    typedef MMU::RV64_Flags RV64_Flags;
    typedef MMU::Page_Table Page_Table;
    typedef MMU::Page_Directory Page_Directory;

public:
    Setup();

private:
    void say_hi();
    void init_mmu();
    void call_next();

private:
    System_Info *si;
};

Setup::Setup()
{
    Display::init();
    kout << endl;
    kerr << endl;

    si = reinterpret_cast<System_Info *>(&__boot_time_system_info);
    if (si->bm.n_cpus > Traits<Machine>::CPUS)
        si->bm.n_cpus = Traits<Machine>::CPUS;

    db<Setup>(TRC) << "Setup(si=" << reinterpret_cast<void *>(si) << ",sp=" << CPU::sp() << ")" << endl;
    db<Setup>(INF) << "Setup:si=" << *si << endl;

    say_hi();      // Print basic facts about this EPOS instance
    init_mmu();    // initializing the MMU
    call_next();   // SETUP ends here, so let's transfer control to the next stage (INIT or APP)
}

void Setup::say_hi()
{
    db<Setup>(TRC) << "Setup::say_hi()" << endl;
    db<Setup>(INF) << "System_Info=" << *si << endl;

    if (si->bm.application_offset == -1U)
        db<Setup>(ERR) << "No APPLICATION in boot image, you don't need EPOS!" << endl;

    kout << "This is EPOS!\n"
         << endl;
    kout << "Setting up this machine as follows: " << endl;
    kout << "  Mode:         " << ((Traits<Build>::MODE == Traits<Build>::LIBRARY) ? "library" : (Traits<Build>::MODE == Traits<Build>::BUILTIN) ? "built-in"
                                                                                                                                                 : "kernel")
         << endl;
    kout << "  Processor:    " << Traits<Machine>::CPUS << " x RV" << Traits<CPU>::WORD_SIZE << " at " << Traits<CPU>::CLOCK / 1000000 << " MHz (BUS clock = " << Traits<CPU>::CLOCK / 1000000 << " MHz)" << endl;
    kout << "  Machine:      SiFive-U" << endl;
    kout << "  Memory:       " << (RAM_TOP + 1 - RAM_BASE) / (1024 * 1024) << " MB [" << reinterpret_cast<void *>(RAM_BASE) << ":" << reinterpret_cast<void *>(RAM_TOP) << "]" << endl;
    kout << "  User memory:  " << (FREE_TOP - FREE_BASE) / (1024 * 1024) << " MB [" << reinterpret_cast<void *>(FREE_BASE) << ":" << reinterpret_cast<void *>(FREE_TOP) << "]" << endl;
    kout << "  I/O space:    " << (MIO_TOP + 1 - MIO_BASE) / (1024 * 1024) << " MB [" << reinterpret_cast<void *>(MIO_BASE) << ":" << reinterpret_cast<void *>(MIO_TOP) << "]" << endl;
    kout << "  Node Id:      ";
    if (si->bm.node_id != -1)
        kout << si->bm.node_id << " (" << Traits<Build>::NODES << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    kout << "  Position:     ";
    if (si->bm.space_x != -1)
        kout << "(" << si->bm.space_x << "," << si->bm.space_y << "," << si->bm.space_z << ")" << endl;
    else
        kout << "will get from the network!" << endl;
    if (si->bm.extras_offset != -1UL)
        kout << "  Extras:       " << si->lm.app_extra_size << " bytes" << endl;

    kout << endl;
}

void Setup::init_mmu()
{
    // Useful variables
    unsigned int PG_SIZE = 4096;
    unsigned int PT_ENTRIES = MMU::PT_ENTRIES;
    unsigned long pages = MMU::pages(RAM_TOP + 1);
    unsigned int L0_ENTRIES = PT_ENTRIES;
    unsigned int L1_ENTRIES = PT_ENTRIES;
    unsigned int L2_ENTRIES = MMU::PD_ENTRIES;

    unsigned int L0_PT_SIZE = L1_ENTRIES * PG_SIZE;
    unsigned int L1_PT_SIZE = L2_ENTRIES * PG_SIZE;
 
    unsigned int L2_TABLES = (pages + L2_ENTRIES - 1) / L2_ENTRIES;
    unsigned int L1_TABLES = (L2_TABLES + L1_ENTRIES - 1) / L1_ENTRIES;
    unsigned int L0_TABLES = (L1_TABLES + L0_ENTRIES - 1) / L0_ENTRIES;

    kout << "Total Pages: " << pages << endl;
    kout << "L0 Page Tables: " << L0_TABLES << endl;
    kout << "L1 Page Tables: " << L1_TABLES << endl;
    kout << "L2 Page Tables: " << L2_TABLES << endl;

    Phy_Addr L0_PT_ADDR = PAGE_TABLES;
    Phy_Addr L1_PT_ADDR = L0_PT_ADDR + L0_TABLES * L0_PT_SIZE;
    Phy_Addr L2_PT_ADDR = L1_PT_ADDR + L1_TABLES * L1_PT_SIZE;

    Page_Directory * master = new ((void *)L2_PT_ADDR) Page_Directory();
    kout << "Master Base Address: " << L2_PT_ADDR << endl;

    master->remap(L1_PT_ADDR, RV64_Flags::V, 0, L1_ENTRIES);

    for (unsigned long i = 0; i < L1_TABLES; i++) {
        Page_Table *pd_lv1 = new ((void *)L1_PT_ADDR) Page_Table();
        L1_PT_ADDR += L1_PT_SIZE;

        pd_lv1->remap(L0_PT_ADDR, RV64_Flags::V, 0, L0_ENTRIES);

        for (unsigned long j = 0; j < L0_TABLES; j++) {
            Page_Table *pt_lv0 = new ((void *)L0_PT_ADDR) Page_Table();
            L0_PT_ADDR += L0_PT_SIZE;

            pt_lv0->remap(j * L0_ENTRIES * PG_SIZE, RV64_Flags::V, 0, L0_ENTRIES);
        }
    }

    kout << "L0 Page Table Address: " << L0_PT_ADDR << endl;
    kout << "L1 Page Table Address: " << L1_PT_ADDR << endl;
    kout << "L2 Page Table End Address: " << L2_PT_ADDR <<endl;

    db<Setup>(INF) << "Set SATP" << endl;
    CPU::satp((1UL << 63) | (reinterpret_cast<unsigned long>(master) >> 12)); // Set SATP and enable paging
    kout << "satp value: " << master << endl << "Trying to flush the TLB..." << endl;

    db<Setup>(INF) << "Flush TLB" << endl;
    MMU::flush_tlb(); // Flush TLB to ensure we've got the right memory organization
    kout << "TLB Flush end" << endl;
}

void Setup::call_next()
{
    db<Setup>(INF) << "SETUP almost ready" << endl;

    CPU::sie(CPU::SSI | CPU::STI | CPU::SEI);
    CPU::sstatus(CPU::SPP_S);

    CPU::sepc(CPU::Reg(&_start));
    CLINT::stvec(CLINT::DIRECT, CPU::Reg(&_int_entry));

    CPU::sret();
    db<Setup>(ERR) << "OS failed to init!" << endl;
}

__END_SYS

using namespace EPOS::S;

void _entry() // machine mode
{
    // SiFive-U core 0 doesn't have MMU, so we halt it
    if (CPU::mhartid() == 0)
        CPU::halt();

    // ensure that sapt is 0
    CPU::satp(0);
    Machine::clear_bss();

    // set the stack pointer, thus creating a stack for SETUP
    CPU::sp(Memory_Map::BOOT_STACK + Traits<Machine>::STACK_SIZE - sizeof(long));

    // Set up the Physical Memory Protection registers correctly
    CPU::pmpcfg0(0x1f);             // A = NAPOT, X, R, W
    CPU::pmpaddr0((1UL << 55) - 1); // All memory

    // Delegate all traps to supervisor except for the timer (architecture decisions)
    CPU::mideleg(CPU::SSI | CPU::STI | CPU::SEI);
    CPU::medeleg(0xffff);

    CPU::mies(CPU::MSI | CPU::MTI | CPU::MEI);              // enable interrupts generation by CLINT
    CPU::mint_disable();                                    // (mstatus) disable interrupts (they will be reenabled at Init_End)
    CLINT::mtvec(CLINT::DIRECT, CPU::Reg(&_mmode_forward)); // setup a preliminary machine mode interrupt handler pointing it to _mmode_forward

    // MPP_S = change to supervirsor
    // MPIE  = otherwise we won't ever receive interrupts
    CPU::mstatus(CPU::MPP_S | CPU::MPIE);
    CPU::mepc(CPU::Reg(&_setup));           // entry = _setup
    CPU::mret();                            // enter supervisor mode at setup (mepc) with interrupts enabled (mstatus.mpie = true)
}

void _setup() // supervisor mode
{
    kerr << endl;
    kout << endl;

    Setup setup;
}
