// arch/riscv/kernel/proc.c
#include "defs.h"
#include "mm.h"
#include "proc.h"
#include "printk.h"
#include "rand.h"

extern void __dummy();
extern char swapper_pg_dir[];
extern char uapp_start[];
extern char uapp_end[];

/* 线程初始化 创建 NR_TASKS 个线程 */
void task_init()
{
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    idle = (struct task_struct *)kalloc();
    // 2. 设置 state 为 TASK_RUNNING;
    idle->state = TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    idle->counter = 0;
    idle->priority = 0;
    // 4. 设置 idle 的 pid 为 0
    idle->pid = 0;
    // 5. 将 current 和 task[0] 指向 idle
    current = idle;
    task[0] = idle;
    /* YOUR CODE HERE */

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, counter 为 0, priority 使用 rand() 来设置, pid 为该线程在线程数组中的下标。
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址

    /* YOUR CODE HERE */
    for (int i = 1; i < NR_TASKS; i++)
    {
        task[i] = (struct task_struct *)kalloc();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 0;
        task[i]->priority = rand();
        task[i]->pid = i;

        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE;

        //set S-Mode and U-Mode stack
        //一个坑,thread_info没有分配内存。
        //解决方案:给一个page(不过有点浪费QAQ)
        task[i]->thread_info = (struct thread_info*)alloc_page();
        task[i]->thread_info->kernel_sp = task[i]->thread.sp;
        // task[i]->thread_info->kernel_sp = (uint64)task[i] + PGSIZE;
        task[i]->thread_info->user_sp = alloc_page();

        //为每个用户态创建自己的页表
        pagetable_t pageTable = alloc_page();
        //复制内核页表，避免在陷入切换U/S-Mode时需要立即更换satp
        for (uint64 i = 0; i < PGSIZE; i++) 
            ((char*)pageTable)[i] = ((char*)swapper_pg_dir)[i];

        //映射uapp,给予XWR权限,UV也为1
        uint64 virtualAddress = USER_START;
        uint64 physicalAddress = (uint64)(uapp_start)-PA2VA_OFFSET;
        create_mapping(pageTable, virtualAddress, physicalAddress, (uint64)(uapp_end)-(uint64)(uapp_start), 31);

        //映射U-Mode Stack,给予WR权限,UV也为1
        virtualAddress = USER_END-PGSIZE;
        physicalAddress = task[i]->thread_info->user_sp-PA2VA_OFFSET;
        create_mapping(pageTable, virtualAddress, physicalAddress, PGSIZE, 23);

        //set sepc
        task[i]->thread.sepc = USER_START;

        //set sstatus
        uint64 pre_sstatus = csr_read(sstatus);
        uint64 now_sstatus = pre_sstatus & 0xfffffffffffffeff; //sstatus[SPP] = 0
        now_sstatus = now_sstatus | (1<<5); //sstatus[SPIE] = 1
        now_sstatus = now_sstatus | (1<<18); //sstatus[SUM] = 1
        task[i]->thread.sstatus = now_sstatus;

        //set sscratch
        task[i]->thread.sscratch = USER_END;

        //设置pgd,即satp
        uint64 pre_satp = csr_read(satp);
        uint64 satp_prefix = (pre_satp >> 44) << 44;
        uint64 now_satp = satp_prefix | (((uint64)pageTable-PA2VA_OFFSET) >> 12);
        task[i]->pgd = now_satp;

    }

    printk("...proc_init done!\n");
}

/*判断下一个执行的线程 next 与当前的线程 current 是否为同一个线程, 如果是同一个线程, 则无需做任何处理, 否则调用 __switch_to 进行线程切换。*/
extern void __switch_to(struct task_struct *prev, struct task_struct *next);
/* 线程切换入口函数*/
void switch_to(struct task_struct *next)
{
    /* YOUR CODE HERE */
    if (current->pid != next->pid)
    {
        //由于__switch_to 中改了ra,导致无法ret回来执行，所以我们需要先把next赋值给current
        struct task_struct* tmp = current;
        current = next;
        __switch_to(tmp, next);
    }
}

/* 在时钟中断处理中被调用 用于判断是否需要进行调度 */
void do_timer(void)
{
    // 1. 如果当前线程是 idle 线程 直接进行调度
    if (current->pid == idle->pid)
    {
        schedule();
    }
    else
    {
        // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度
        current->counter--;
        if(current->counter<=0){
            schedule();
        }
    }
}

/* dummy funciton: 一个循环程序, 循环输出自己的 pid 以及一个自增的局部变量 */
void dummy()
{
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while (1)
    {
        if (last_counter == -1 || current->counter != last_counter)
        {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d\n", current->pid, auto_inc_local_var);
        }
    }
}

/* 调度程序 选择出下一个运行的线程 */
void schedule(void)
{
    /* YOUR CODE HERE */
    // SJF

    // WARNING: 这里的代码基于所有线程只有一种状态。若所有线程不止RUNNING状态，则需更改逻辑
   

    #ifdef SJF
    uint64 less = NR_TASKS;
    for(int i = 1; i < NR_TASKS; i++){
        if(task[i]->counter > 0){
            less = i;
            break;
        } 
    }
    for (int i = less + 1; i < NR_TASKS; i++)
    {
        if (task[i]->counter != 0 && task[i]->counter < task[less]->counter)
            less = i;
    }
    //找了一圈还是没有
    if (less == NR_TASKS)
    {
        //重置
        for (int i = 1; i < NR_TASKS; i++)
        {
            task[i]->counter = rand();
            printk("SET [PID = %d COUNTER = %d]\n", i, task[i]->counter);
        }
        //重新选择less
        for(int i = 1; i < NR_TASKS; i++){
            if(task[i]->counter > 0){
                less = i;
                break;
            } 
        }
        for (int i = less + 1; i < NR_TASKS; i++)
        {
            if (task[i]->counter != 0 && task[i]->counter < task[less]->counter)
                less = i;
        }
    }
    //DEBUG: 打印线程信息
    // for(int i = 1; i < NR_TASKS; i++){
    //     printk("pid=%d counter=%d\n",task[i]->pid, task[i]->counter);
    // }
    printk("using SJF, switch to [PID = %d COUNTER = %d]\n",task[less]->pid, task[less]->counter);
    switch_to(task[less]);
    #endif

    #ifdef PRIORITY
     uint64 less = NR_TASKS;
    for(int i = 1; i < NR_TASKS; i++){
        if(task[i]->counter > 0){
            less = i;
            break;
        } 
    }
    for (int i = less + 1; i < NR_TASKS; i++)
    {
        if (task[i]->counter != 0 && task[i]->priority > task[less]->priority)
            less = i;
    }
    //若所有线程counter都已经用尽，重新分配counter与priority
    if (less == NR_TASKS)
    {
        //重置
        for (int i = 1; i < NR_TASKS; i++)
        {
            task[i]->counter = rand();
            task[i]->priority = rand();
            printk("SET [PID = %d PRIORITY = %d COUNTER = %d]\n", i, task[i]->priority, task[i]->counter);
        }
        //重新选择less
        for(int i = 1; i < NR_TASKS; i++){
            if(task[i]->counter > 0){
                less = i;
                break;
            } 
        }
        for (int i = less + 1; i < NR_TASKS; i++)
        {
            if (task[i]->counter != 0 && task[i]->priority > task[less]->priority)
                less = i;
        }
    }
    //DEBUG: 打印线程信息
    // for(int i = 1; i < NR_TASKS; i++){
    //     printk("pid=%d PRIORITY = %d counter=%d\n",task[i]->pid,task[i]->priority, task[i]->counter);
    // }
    printk("using PRIORITY, switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",task[less]->pid, task[less]->priority, task[less]->counter);
    switch_to(task[less]);
    #endif
}
