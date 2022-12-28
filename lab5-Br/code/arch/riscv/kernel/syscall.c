#include "../include/syscall.h"
#include "defs.h"
#include "printk.h"
#include "proc.h"

// 系统调用号使用a7,入参a0~a5,返回值a0/a1
void syscall(struct pt_regs *regs, uint64 sepc){
    if(regs->reg[17] == 64){    //sys_write
        sys_write(regs,regs->reg[10],regs->reg[11],regs->reg[12]);
    }
    else if(regs->reg[17] == 172){  //sys_getpid
        sys_getpid(regs);
    }
    else{
        printk("Other syscall occured! a7=%d\n",regs->reg[17]);
    }

}

void sys_write(struct pt_regs *regs, unsigned int fd, const char* buf, size_t count){
    if (fd == 1) {
        uint64 return_value;
        for(size_t i = 0; i < count; i++){
            return_value = printk("%c", buf[i]);
        }
        regs->reg[10] = return_value;
    }
}

void sys_getpid(struct pt_regs *regs){
    regs->reg[10] = current->pid;
}