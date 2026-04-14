// Put a 64-bit value with little endianness.
.macro PUT_64B high, low
.word \low
.word \high
.endm
// Create an entry pointing to a next-level table.
.macro TABLE_ENTRY PA, ATTR
    PUT_64B \ATTR, (\PA) + 0x3
.endm
// Create an entry for a 1GB block.
.macro BLOCK_1GB PA, ATTR_HI, ATTR_LO
    PUT_64B \ATTR_HI, ((\PA) & 0xC0000000) | \ATTR_LO | 0x1
.endm
// Create an entry for a 2MB block.
.macro BLOCK_2MB PA, ATTR_HI, ATTR_LO
    PUT_64B \ATTR_HI, ((\PA) & 0xFFE00000) | \ATTR_LO | 0x1
.endm
.align 12
ttb0_base:
    TABLE_ENTRY level2_pagetable, 0
    BLOCK_1GB 0x40000000, 0, 0x740
    BLOCK_1GB 0x80000000, 0, 0x740
    BLOCK_1GB 0xC0000000, 0, 0x740
.align 12
level2_pagetable:
.set ADDR, 0x000 // The current page address.
.rept 0x200
    BLOCK_2MB (ADDR << 20), 0, 0x74C
.set ADDR, ADDR+2
.endr
.global _Reset

_Reset:
    // Initialize the stack pointer.
    LDR R13, =stack_top
    ADD R13, R13, #4
    MRC P15, 0, R0, C0, C0, 5 // Read MPIDR.
    AND R0, R0, #0xFF // R0 == core number.
    MOV R2, #0x1000
    MUL R1, R0, R2 // Create separate stack spaces
    SUB R13, R13, R1 // for each processor.
    // Initialize SPSR in all modes.
    MOV R0, #0
    MSR SPSR, R0
    MSR SPSR_svc, R0
    // Disable L1 Caches.
    MRC P15, 0, R1, C1, C0, 0 // Read SCTLR.
    BIC R1, R1, #(0x1 << 2) // Disable D Cache.
    MCR P15, 0, R1, C1, C0, 0 // Write SCTLR.
    // Invalidate Data cache to create general-purpose code. Calculate the
    // cache size first and loop through each set + way.
    MOV R0, #0x0 // R0 = 0x0 for L1 dcache 0x2 for L2 dcache.
    MCR P15, 2, R0, C0, C0, 0 // CSSELR Cache Size Selection Register.
    MRC P15, 1, R4, C0, C0, 0 // CCSIDR read Cache Size.
    AND R1, R4, #0x7
    ADD R1, R1, #0x4 // R1 = Cache Line Size.
    LDR R3, =0x7FFF
    AND R2, R3, R4, LSR #13 // R2 = Cache Set Number – 1.
    LDR R3, =0x3FF
    AND R3, R3, R4, LSR #3 // R3 = Cache Associativity Number – 1.
    CLZ R4, R3 // R4 = way position in CISW instruction.
    MOV R5, #0 // R5 = way loop counter.
way_loop:
    MOV R6, #0 // R6 = set loop counter.
set_loop:
    ORR R7, R0, R5, LSL R4 // Set way.
    ORR R7, R7, R6, LSL R1 // Set set.
    MCR P15, 0, R7, C7, C6, 2 // DCCISW R7.
    ADD R6, R6, #1 // Increment set counter.
    CMP R6, R2 // Last set reached yet?
    BLE set_loop // If not, iterate set_loop,
    ADD R5, R5, #1 // else, next way.
    CMP R5, R3 // Last way reached yet?
    BLE way_loop // if not, iterate way_loop.
    // Initialize TTBCR.
    MOV R0, #0 // Use short descriptor.
    MCR P15, 0, R0, C2, C0, 2 // Base address is 16KB aligned.
    // Perform translation table walk for TTBR0.
    // Initialize DACR.
    LDR R1, =0x55555555 // Set all domains as clients.
    MCR P15, 0, R1, C3, C0, 0 // Accesses are checked against the
    // permission bits in the translation tables.
    // Initialize SCTLR.AFE.
    MRC P15, 0, R1, C1, C0, 0 // Read SCTLR.
    BIC R1, R1, #(0x1 <<29) // Set AFE to 0 and disable Access Flag.
    MCR P15, 0, R1, C1, C0, 0 // Write SCTLR.
    // Initialize TTBR0.
    LDR R0, =_page_dir_core0//ttb0_base // ttb0_base must be a 16KB-aligned address.

    @ MOV R1, #0x2B // The translation table walk is normal, inner
    @ ORR R1, R0, R1 // and outer cacheable, WB WA, and inner
    @ MCR P15, 0, R1, C2, C0, 0 // shareable.
    
    MOV R1, #0x6A // The translation table walk is normal, inner
    ORR R1, R0, R1 // and outer cacheable, WB WA, and inner
    MCR P15, 0, R1, C2, C0, 0 // shareable.


    // Set up translation table entries in memory
    LDR R4, =0x00100000 // Increase 1MB address each time.
    LDR R2, =0x00015C06 // Set up translation table descriptor with
    // Secure, global, full accessibility,
    // executable.
    // Domain 0, Shareable, Normal cacheable memory
    LDR R3, =256 // executes the loop 256 times to set up
    // 256 descriptors to cover 0-256MB memory.

loop:
    STR R2, [R0], #4 // Build a page table section entry.
    ADD R2, R2, R4 // Update address part for next descriptor.
    SUBS R3, #1
    BNE loop

    LDR R3, =1792
    LDR R2, =0x10010C02
loopx:
    STR R2, [R0], #4
    ADD R2, R2, R4 
    SUBS R3, #1
    BNE loopx

    LDR R2, =0x80015C06 // Set up translation table descriptors with
    // secure, global, full accessibility,
    // Domain=0 Shareable Device-nGnRnE Memory.
    LDR R3, =2048 // Executes loop 3072 times to set up 3072
loop2:
    STR R2, [R0], #4 // Build a translation table section entry.
    ADD R2, R2, R4 // Update address part for next descriptor.
    SUBS R3, #1
    BNE loop2
    // SMP is implemented in the CPUECTLR register.
    MRRC P15, 1, R0, R1, C15 // Read CPUECTLR.
    ORR R0, R0, #(0x1 << 6) // Set SMPEN.
    MCRR P15, 1, R0, R1, C15 // Write CPUECTLR.
    // Enable caches and the MMU.
    MRC P15, 0, R1, C1, C0, 0 // Read SCTLR.
    ORR R1, R1, #(0x1 << 2) // The C bit (data cache).
    ORR R1, R1, #(0x1 << 12) // The I bit (instruction cache).
    ORR R1, R1, #0x1 // The M bit (MMU).
    MCR P15, 0, R1, C1, C0, 0 // Write SCTLR.
    DSB
    ISB
    BL main
    B .
