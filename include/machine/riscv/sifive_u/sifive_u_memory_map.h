// EPOS SiFive-U (RISC-V) Memory Map

#ifndef __riscv_sifive_u_memory_map_h
#define __riscv_sifive_u_memory_map_h

#include <system/memory_map.h>

__BEGIN_SYS

struct Memory_Map
{
private:
    static const bool multitask = false;
    static const bool emulated = (Traits<CPU>::WORD_SIZE != 64); // specifying a SiFive-U with RV64 sets QEMU machine to Virt

public:
    enum : unsigned long {
        NOT_USED        = Traits<Machine>::NOT_USED,

        // Physical Memory
        RAM_BASE        = Traits<Machine>::RAM_BASE,
        RAM_TOP         = Traits<Machine>::RAM_TOP,
        MIO_BASE        = Traits<Machine>::MIO_BASE,
        MIO_TOP         = Traits<Machine>::MIO_TOP,
        BOOT_STACK      = RAM_TOP,                                     // will be used as the stack pointer
        PAGE_TABLES     = BOOT_STACK - 64 * 1024 - ((1 + 512 + (512*512)) * 4096) + 1,
        FREE_BASE       = RAM_BASE,
        FREE_TOP        = BOOT_STACK,

        // Memory-mapped devices
        BIOS_BASE       = 0x0000000000001000,   // BIOS ROM
        TEST_BASE       = 0x0000000000100000,   // SiFive test engine
        RTC_BASE        = 0x0000000000101000,   // Goldfish RTC
        UART0_BASE      = emulated ? 0x0000000010000000 : 0x0000000010010000, // NS16550A or SiFive UART
        CLINT_BASE      = 0x0000000002000000,   // SiFive CLINT
        TIMER_BASE      = 0x0000000002004000,   // CLINT Timer
        PLIIC_CPU_BASE  = 0x000000000c000000,   // SiFive PLIC
        PRCI_BASE       = emulated ? NOT_USED : 0x0000000010000000,   // SiFive-U Power, Reset, Clock, Interrupt
        GPIO_BASE       = emulated ? NOT_USED : 0x0000000010060000,   // SiFive-U GPIO
        OTP_BASE        = emulated ? NOT_USED : 0x0000000010070000,   // SiFive-U OTP
        ETH_BASE        = emulated ? NOT_USED : 0x0000000010090000,   // SiFive-U Ethernet
        FLASH_BASE      = 0x0000000020000000,   // Virt / SiFive-U Flash

        // Physical Memory at Boot
        BOOT            = Traits<Machine>::BOOT,
        IMAGE           = Traits<Machine>::IMAGE,
        SETUP           = Traits<Machine>::SETUP,

        // Logical Address Space
        APP_LOW         = Traits<Machine>::APP_LOW,
        APP_CODE        = Traits<Machine>::APP_CODE,
        APP_DATA        = Traits<Machine>::APP_DATA,
        APP_HEAP        = Traits<Machine>::APP_HEAP,
        APP_HIGH        = Traits<Machine>::APP_HIGH,

        INIT            = Traits<Machine>::INIT,

        PHY_MEM         = Traits<Machine>::PHY_MEM,

        IO              = Traits<Machine>::IO,

        SYS = Traits<Machine>::SYS,
        SYS_CODE = SYS,
        SYS_DATA = Traits<Machine>::SYS_DATA,
        SYS_INFO = PAGE_TABLES - 4096,
        MMODE_F  = SYS_INFO - 4096,
        SYS_STACK = SYS_DATA + 4 * 1024 * 1024,
        SYS_HEAP = Traits<Machine>::SYS_HEAP,
        SYS_HIGH = Traits<Machine>::SYS_HIGH

        // SYS             = Traits<Machine>::SYS,
        // SYS_CODE        = multitask ? SYS + 0x00000000 : NOT_USED,
        // // SYS_INFO        = multitask ? SYS + 0x00100000 : NOT_USED,
        // SYS_INFO = PAGE_TABLES - 4096,
        // MMODE_F  = SYS_INFO - 4096,
        // SYS_PT          = multitask ? SYS + 0x00101000 : NOT_USED,
        // SYS_PD          = multitask ? SYS + 0x00102000 : NOT_USED,
        // SYS_DATA        = multitask ? SYS + 0x00103000 : NOT_USED,
        // SYS_STACK       = multitask ? SYS + 0x00200000 : NOT_USED,
        // SYS_HEAP        = multitask ? SYS + 0x00400000 : NOT_USED,
        // SYS_HIGH        = multitask ? SYS + 0x007fffff : NOT_USED

    };
};

__END_SYS

#endif
