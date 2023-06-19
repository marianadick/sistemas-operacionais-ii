// EPOS RISC-V 32 MMU Mediator Implementation

#include <architecture/rv32/rv32_mmu.h>

__BEGIN_SYS

SV32_MMU::List SV32_MMU::_free[colorful * COLORS + 1];
SV32_MMU::Page_Directory * SV32_MMU::_master;

__END_SYS
