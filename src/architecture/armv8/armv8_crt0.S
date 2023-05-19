// EPOS ARMv8 Program Starter

        .file "armv8_crt0.S"

        // Entry point called by SETUP for processors
        .section .text
        .align  8
        .global _start
        .type   _start, function
_start:
        // Temporary stack(s) for INIT were created and configure by SETUP
        // BSS was cleared by SETUP
        
        stp     x0,  x1, [sp, #-16]!    // save argc and argv
        bl      _init

        // Call main
        .align  8
        .globl  __epos_app_entry
        .type   __epos_app_entry, function
__epos_app_entry:
        // Main's stack was allocated by Thread::init()
        ldp    x0,  x1, [sp], #16       // restore argc and argv saved before calling _init
        bl      main
        str     x0, [sp, #-8]!	// save main's return value to be used by exit()
        bl      _fini
        bl      _exit
