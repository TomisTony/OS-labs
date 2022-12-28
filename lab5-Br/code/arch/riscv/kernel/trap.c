#include "printk.h"
#include "proc.h"
#include "../include/syscall.h"


void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs)
{
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.5 节
    // 其他interrupt / exception 可以直接忽略

    // YOUR CODE HERE
    unsigned long timer = 0x8000000000000005;
    unsigned long syscall_flag = 0x8;    //environment-call-from-U-mode
    // interrupt or exception
    if (scause == timer)
    {
        clock_set_next_event();
        do_timer();
        
    }
    else if(scause == syscall_flag){
        syscall(regs, sepc);
    }
}
