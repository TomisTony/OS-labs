    .section .text.entry
    .align 2
    .globl _traps 
    .globl __dummy
    .globl __switch_to
__switch_to:
    # save state to prev process
    # YOUR CODE HERE
    sd ra, 40(a0)
    sd sp, 48(a0)
    sd s0, 56(a0)
    sd s1, 64(a0)
    sd s2, 72(a0)
    sd s3, 80(a0)
    sd s4, 88(a0)
    sd s5, 96(a0)
    sd s6, 104(a0)
    sd s7, 112(a0)
    sd s8, 120(a0)
    sd s9, 128(a0)
    sd s10, 136(a0)
    sd s11, 144(a0)
    # save csr
    csrr t0, sepc
    sd t0, 152(a0)
    csrr t0, sstatus
    sd t0, 160(a0)
    csrr t0, sscratch
    sd t0, 168(a0)
    csrr t0, satp
    sd t0, 176(a0)

    # restore state from next process
    # YOUR CODE HERE
    ld ra, 40(a1)
    ld sp, 48(a1)
    ld s0, 56(a1)
    ld s1, 64(a1)
    ld s2, 72(a1)
    ld s3, 80(a1)
    ld s4, 88(a1)
    ld s5, 96(a1)
    ld s6, 104(a1)
    ld s7, 112(a1)
    ld s8, 120(a1)
    ld s9, 128(a1)
    ld s10, 136(a1)
    ld s11, 144(a1)
    # write csr
    ld t0, 152(a1)
    csrw sepc, t0
    ld t0, 160(a1)
    csrw sstatus, t0
    ld t0, 168(a1)
    csrw sscratch, t0
    ld t0, 176(a1)
    csrw satp, t0

    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i

    ret

__dummy:
    # la t0, dummy
    # csrw sepc, t0;
    # 不需要设置sepc,因为我们不再去dummy()了
    # 而且__swich_to已经设置好了我们要去的sepc(USER_START)

    # 交换sp和sscratch来切换U-stack和S-stack
    csrr t0, sscratch
    csrw sscratch, sp
    mv sp, t0
    sret

_traps:
    # 检查sscratch来确定是不是内核线程
    csrr t0, sscratch
    # 是内核线程则跳过切换栈阶段
    beq t0, zero, _get_in_trap
    # 切换栈
    csrr t0, sscratch
    csrw sscratch, sp
    mv sp, t0
_get_in_trap:
    # 1. save 32 registers, sepc and sstatus to stack
    addi sp,sp,-272
    # 保存sepc,首先保存t0
    sd x5,40(sp)
    csrr t0,sepc
    sd x5,256(sp)
    sd x0,0(sp)
    sd x1,8(sp)
    sd x2,16(sp)
    sd x3,24(sp)
    sd x4,32(sp)
    sd x6,48(sp)
    sd x7,56(sp)
    sd x8,64(sp)
    sd x9,72(sp)
    sd x10,80(sp)
    sd x11,88(sp)
    sd x12,96(sp)
    sd x13,104(sp)
    sd x14,112(sp)
    sd x15,120(sp)
    sd x16,128(sp)
    sd x17,136(sp)
    sd x18,144(sp)
    sd x19,152(sp)
    sd x20,160(sp)
    sd x21,168(sp)
    sd x22,176(sp)
    sd x23,184(sp)
    sd x24,192(sp)
    sd x25,200(sp)
    sd x26,208(sp)
    sd x27,216(sp)
    sd x28,224(sp)
    sd x29,232(sp)
    sd x30,240(sp)
    sd x31,248(sp)
    # 由于pt_regs,加存sstatus
    csrr t0, sstatus
    sd t0,264(sp)

    # 2. call trap_handler
    csrr a0, scause
    csrr a1, sepc
    mv a2, sp
    call trap_handler

    # 3. restore sstatus, sepc and 32 registers (x2(sp) should be restore last) from stack
    # 首先使用t0取出sepc和sstatus
    ld t0,264(sp)
    csrw sstatus,t0
    ld t0,256(sp)
    # 如果不是系统调用，那就不用给sepc加上4了
    csrr t1, scause
    addi t2, zero, 0x8
    bne t1, t2, _no_need_change_sepc
    addi t0,t0,0x4
_no_need_change_sepc: 
    csrw sepc,t0

    ld x0,0(sp)
    ld x1,8(sp)
    ld x3,24(sp)
    ld x4,32(sp)
    ld x5,40(sp)
    ld x6,48(sp)
    ld x7,56(sp)
    ld x8,64(sp)
    ld x9,72(sp)
    ld x10,80(sp)
    ld x11,88(sp)
    ld x12,96(sp)
    ld x13,104(sp)
    ld x14,112(sp)
    ld x15,120(sp)
    ld x16,128(sp)
    ld x17,136(sp)
    ld x18,144(sp)
    ld x19,152(sp)
    ld x20,160(sp)
    ld x21,168(sp)
    ld x22,176(sp)
    ld x23,184(sp)
    ld x24,192(sp)
    ld x25,200(sp)
    ld x26,208(sp)
    ld x27,216(sp)
    ld x28,224(sp)
    ld x29,232(sp)
    ld x30,240(sp)
    ld x31,248(sp)
    ld x2,16(sp)
    addi sp,sp,272

    # 检查sscratch来确定是不是内核线程
    csrr t0, sscratch
    # 是内核线程则跳过切换栈阶段
    beq t0, zero, _return_from_trap
    # 切换栈
    csrr t0, sscratch
    csrw sscratch, sp
    mv sp, t0
_return_from_trap:
    # 4. return from trap
    sret

