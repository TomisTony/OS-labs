#include "printk.h"
#include "sbi.h"
#include "defs.h"
#include "proc.h"

extern void test();

int start_kernel() {
    printk("%d",2022);
    printk("%s"," Hello RISC-V\n");
    //直接开始调度
    schedule();

    test(); // DO NOT DELETE !!!

	return 0;
}
