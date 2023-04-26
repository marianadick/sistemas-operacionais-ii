// EPOS SiFive-U (RISC-V) Metainfo and Configuration

#ifndef __riscv_sifive_u_traits_h
#define __riscv_sifive_u_traits_h

#include <system/config.h>

__BEGIN_SYS

class Machine_Common;
template<> struct Traits<Machine_Common>: public Traits<Build>
{
protected:
    static const bool library = (Traits<Build>::MODE == Traits<Build>::LIBRARY);
};

template<> struct Traits<Machine>: public Traits<Machine_Common>
{
public:
    // Value to be used for undefined addresses
    static const unsigned long NOT_USED          = 0xffffffffffffffff;                  // Setting 64 bits as the standart for the APP

    // 'Global' default sizes and quantities
    static const unsigned long MAX_THREADS       = 16;
    static const unsigned long STACK_SIZE        = 0x10000;                             // 64 kB
    static const unsigned long HEAP_SIZE         = 0x100000;                            // 1 MB

    // General system infos
    static const unsigned long SYS               = 0xffffffff80200000;
    static const unsigned long SYS_CODE          = NOT_USED;    
    static const unsigned long SYS_INFO          = NOT_USED;
    static const unsigned long SYS_PT            = NOT_USED;
    static const unsigned long SYS_PD            = NOT_USED;
    static const unsigned long SYS_DATA          = 0xffffffff80400000;
    static const unsigned long SYS_STACK         = SYS_DATA + 0x400000;                 // 4 MB
    static const unsigned long SYS_HEAP          = 0xffffffff81c00000;
    static const unsigned long SYS_HIGH          = 0xffffffff82000000 - 1;

    // Clocks
    static const unsigned long CLOCK             = 1000000000;                           // CORECLK
    static const unsigned long HFCLK             =   33330000;                           // FU540-C000 generates all internal clocks from 33.33 MHz hfclk driven from an external oscillator (HFCLKIN) or crystal (HFOSCIN) input, selected by input HFXSEL.
    static const unsigned long RTCCLK            =    1000000;                           // The CPU real time clock (rtcclk) runs at 1 MHz and is driven from input pin RTCCLKIN. This should be connected to an external oscillator.
    static const unsigned long TLCLK             = CLOCK / 2;                            // L2 cache and peripherals such as UART, SPI, I2C, and PWM operate in a single clock domain (tlclk) running at coreclk/2 rate. There is a low-latency 2:1 crossing between coreclk and tlclk domains.

    // Physical Memory
    static const unsigned long RAM_BASE          = 0x0000000080000000;                   // 2 GB
    static const unsigned long RAM_TOP           = 0x000000027fffffff;                   // 2 GB RAM (TOP - BASE = 2GB)
    static const unsigned long MIO_BASE          = 0x0000000000000000;
    static const unsigned long MIO_TOP           = 0x000000001fffffff;                   // 512 MB

    static const unsigned long BOOT_STACK        = RAM_TOP + 1 - STACK_SIZE;                            // 64 kB stack's base
    static const unsigned long PAGE_TABLES       = BOOT_STACK - 64 * 1024 - ((1 + 512 + (512*512)) * 4096) + 1;
    static const unsigned long FREE_BASE         = RAM_BASE;                                            // Free memory from RAM_BASE
    static const unsigned long FREE_TOP          = BOOT_STACK;

    // Physical Memory at Boot
    static const unsigned long BOOT              = NOT_USED;
    static const unsigned long SETUP             = library ? NOT_USED : RAM_BASE;           // RAM_BASE (will be part of the free memory at INIT, using a logical address identical to physical eliminate SETUP relocation)
    static const unsigned long IMAGE             = library ? NOT_USED : 0x0000000080100000; // RAM_BASE + 1 MB (will be part of the free memory at INIT, defines the maximum image size; if larger than 3 MB then adjust at SETUP)
    static const unsigned long INIT              = library ? NOT_USED : 0xffffffff80000000; // RAM_BASE + 512 KB (will be part of the free memory at INIT)
    static const unsigned long IO                = 0x0000000000000000; 

    // Logical Memory
    // In Sv39, all bits from bit 39 to bit 63 must be equal to the bit 38
    static const unsigned long APP_LOW           = 0x0000000080000000;                  // 2 GB
    static const unsigned long APP_HIGH          = 0x0000003fffffffff;                  // 256 GB
    static const unsigned long APP_CODE          = APP_LOW;                             // 2 GB
    static const unsigned long APP_DATA          = APP_CODE + 0x400000;                 // APP_CODE + 4 MB
    
    static const unsigned long PHY_MEM           = 0x0000004000000000;                   
};

template <> struct Traits<IC>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;
};

template <> struct Traits<Timer>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    static const unsigned int UNITS = 1;
    static const unsigned int CLOCK = Traits<Machine>::RTCCLK;

    // Meaningful values for the timer frequency range from 100 to 10000 Hz. The
    // choice must respect the scheduler time-slice, i. e., it must be higher
    // than the scheduler invocation frequency.
    static const int FREQUENCY = 1000; // Hz
};

template <> struct Traits<UART>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 2;

    static const unsigned int CLOCK = Traits<Machine>::TLCLK;

    static const unsigned int DEF_UNIT = 1;
    static const unsigned int DEF_BAUD_RATE = 115200;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_PARITY = 0; // none
    static const unsigned int DEF_STOP_BITS = 1;
};

template <> struct Traits<SPI>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 3;

    static const unsigned int CLOCK = Traits<Machine>::TLCLK;

    static const unsigned int DEF_UNIT = 0;
    static const unsigned int DEF_PROTOCOL = 0;
    static const unsigned int DEF_MODE = 0;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_BIT_RATE = 250000;
};

template<> struct Traits<Serial_Display>: public Traits<Machine_Common>
{
    static const bool enabled = (Traits<Build>::EXPECTED_SIMULATION_TIME != 0);
    static const int ENGINE = UART;
    static const int UNIT = 1;
    static const int COLUMNS = 80;
    static const int LINES = 24;
    static const int TAB_SIZE = 8;
};

template<> struct Traits<Scratchpad>: public Traits<Machine_Common>
{
    static const bool enabled = false;
};

__END_SYS

#endif
