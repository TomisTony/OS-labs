// arch/riscv/kernel/proc.c
#include "defs.h"
#include "mm.h"
#include "proc.h"
#include "printk.h"
#include "rand.h"

extern void __dummy();

struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 `task_struct`
struct task_struct *task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

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
