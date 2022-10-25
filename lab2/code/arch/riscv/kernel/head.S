.extern start_kernel

    .section .text.init
    .globl _start
_start:
    # ------------------
        # set stvec = _traps
        la t0, _traps
        csrw stvec, t0

    # ------------------
        # set sie[STIE] = 1
        csrr t0, sie
        ori t0,t0,0x20
        csrw sie,t0

    # ------------------
        # set first time interrupt
        rdtime t0
        li t1,10000000
        add t0,t0,t1
        mv a7, zero
        mv a6, zero
        mv a0, t0
        mv a1, zero
        mv a2, zero
        mv a3, zero
        mv a4, zero
        mv a5, zero
        ecall

    # ------------------
        # set sstatus[SIE] = 1
        csrr t0, sstatus
        ori t0,t0,0x2
        csrw sstatus,t0

    # ------------------
    la sp, boot_stack_top
    call start_kernel
    
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size

    .globl boot_stack_top
boot_stack_top: