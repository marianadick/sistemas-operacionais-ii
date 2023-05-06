// EPOS RV64 MMU Mediator Implementation

#include <architecture/rv64/rv64_mmu.h>

__BEGIN_SYS

SV39_MMU::List SV39_MMU::_free[colorful * COLORS + 1];
SV39_MMU::Page_Directory * SV39_MMU::_master;

__END_SYS
