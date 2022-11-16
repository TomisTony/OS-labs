#include "defs.h"
.extern start_kernel
.extern mm_init
.extern early_pgtbl
    .section .text.init
    .globl _start
_start:
        la sp, boot_stack_top
        call setup_vm
        call relocate
        call mm_init
        call task_init
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

        # store a0~a7
        addi sp,sp,-64
        sd a0,0(sp)
        sd a1,8(sp)
        sd a2,16(sp)
        sd a3,24(sp)
        sd a4,32(sp)
        sd a5,40(sp)
        sd a6,48(sp)
        sd a7,56(sp)

        # ecall
        mv a7, zero
        mv a6, zero
        mv a0, t0
        mv a1, zero
        mv a2, zero
        mv a3, zero
        mv a4, zero
        mv a5, zero
        ecall

        # reload a0~a7
        ld a0,0(sp)
        ld a1,8(sp)
        ld a2,16(sp)
        ld a3,24(sp)
        ld a4,32(sp)
        ld a5,40(sp)
        ld a6,48(sp)
        ld a7,56(sp)
        addi sp,sp,64

    # ------------------
        # set sstatus[SIE] = 1
        csrr t0, sstatus
        ori t0,t0,0x2
        csrw sstatus,t0

    # ------------------
        call start_kernel
relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0, PA2VA_OFFSET
    add ra, ra, t0
    add sp, sp, PA2VA_OFFSET
    # set satp with early_pgtbl
    la t0, early_pgtbl
    srli t1, t0, 12
    li t0,8
    slli t1, t1, 60
    add t0,t0,t1
    csrw satp,t0
    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret
    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 4096 # <-- change to your stack size
    .globl boot_stack_top
boot_stack_top: 