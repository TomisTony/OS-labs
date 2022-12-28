typedef unsigned long size_t;
typedef unsigned long uint64;

struct  pt_regs
{
    uint64 reg[32];
    uint64 sepc;
    uint64 sstatus;
};

void syscall(struct pt_regs *regs, uint64 sepc);
void sys_write(struct pt_regs *regs, unsigned int fd, const char* buf, size_t count);
void sys_getpid(struct pt_regs *regs);