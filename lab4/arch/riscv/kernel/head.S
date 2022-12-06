.extern start_kernel
.extern mm_init
.extern early_pgtbl

    .section .text.init
    .globl _start
_start:
        # 修改sp 
        li s0, 0xffffffe000000000
        li s1, 0x0000000080000000
        sub s0, s0, s1
        la sp, boot_stack_top
        sub sp,sp,s0

        call setup_vm
        # 用中断代替等值映射
        call relocate
        call mm_init
        call setup_vm_final
        
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
    #如果不进行等值映射
    la t0, __virtual_switch
    csrw stvec, t0
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)
    li t0, 0xffffffe000000000
    li t1, 0x0000000080000000
    sub t0, t0, t1
    add ra, ra, t0
    addi ra, ra, 4
    add sp, sp, t0

    # set satp with early_pgtbl

    # PPN
    la t1, early_pgtbl
    sub t1,t1,t0
    srli t1, t1, 12
    # MODE
    li t2,8
    slli t2, t2, 60
    # set satp
    add t1,t1,t2
    csrw satp,t1

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