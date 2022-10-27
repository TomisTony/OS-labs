# Lab3：RV64 内核线程调度

## 1. 准备工程

从github上同步文件到docker中

![image-20221026214633270](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026214633270.png)

在``_start` 的适当位置调用 `mm_init`, 来初始化内存管理系统

![image-20221026215002977](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026215002977.png)

## 2. `proc.h` 数据结构定义

在`arch/riscv/include`下创建`proc.h`文件

![image-20221026215513358](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026215513358.png)

## 3. 线程调度功能实现

### 3.1 线程初始化

首先我们需要为每个线程分配一个4KB的物理页，并将`task_struct`指向低地址部分，`sp`指向高地址

![image-20221026221634074](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026221634074.png)

同样地为`task[1]` ~ `task[NR_TASKS - 1]`进行初始化，此时`ra`为``__dummy`, `sp`设置为task[i]+PGSIZE

![image-20221026221921779](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026221921779.png)

### 3.2 `__dummy` 与 `dummy` 介绍

首先在`proc.c`中添加dummy()

![image-20221026230301996](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026230301996.png)

我们需要为线程的第一次调度来提供一个返回函数`__dummy`

![image-20221026230203081](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026230203081.png)

代码中 “la t0, dummy"将 `dummy`函数的起始地址赋值给t0寄存器，再将sepc设置为t0，最后使用sret从中断中返回。

### 3.3 实现线程切换

在`arch/riscv/kernel/proc.c`中添加`switch_to`函数

![image-20221026231622858](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221026231622858.png)

在`entry.S`中实现`__switch_to`, a0与a1寄存器的值是传入的`prev`与`next`指针，因此要想将当前线程的寄存器保存到`thread_struct`中，需要在`prev`与`next`指针上加上40来对应到`thread_struct`中的各个变量。变量对应好后只需要依次存入与读取寄存器的值即可。

```assembly
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
    ret
```

### 3.4 实现调度入口函数

实现`do_timer`

```c
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
```

### 3.5 实现线程调度

#### 3.5.1 SJF

对于SJF, 我们不需要关注`priority`, 只需要按照每个task的`counter`大小来排序即可，每次schedule都找出所有task中`counter`最小且不为零的task来`switch_to`。若所有task的`counter`都为零，则随机重新分配`counter`，继续调度循环。具体代码如下：

```c
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
    //若没有线程比idle(counter=0)大
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
    for(int i = 1; i < NR_TASKS; i++){
        printk("pid=%d counter=%d\n",task[i]->pid, task[i]->counter);
    }
    printk("switch to [PID = %d COUNTER = %d]\n",task[less]->pid, task[less]->counter);
    switch_to(task[less]);
    #endif
```

#### 3.5.2 PRIORITY

相比于SJF, 我们只需将`switch_to`前的判断条件由原本`counter`的大小改为`priority`的大小即可。需要注意的是，决定一个线程执行时间的仍然是他的`counter`。当所有线程的`counter`都变为0后，我们对所有线程的`counter`与`priority`重新赋值，继续循环。

```
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
    for(int i = 1; i < NR_TASKS; i++){
        printk("pid=%d PRIORITY = %d counter=%d\n",task[i]->pid,task[i]->priority, task[i]->counter);
    }
    printk("switch to [PID = %d PRIORITY = %d COUNTER = %d]\n",task[less]->pid, task[less]->priority, task[less]->counter);
    switch_to(task[less]);
    #endif
```

## 4. 编译以及测试

### 4.1 编译

首先修改顶层的`Makefile`, 更改

```makefile
CFLAG = ${CF} ${INCLUDE} -D SJF
```

来选择调度算法为`PRIORITY`。

若想使用`SJF`，则可以通过修改顶层`Makefile`, 将`CFLAG`赋值为PRIORITY。

```makefile
CFLAG = ${CF} ${INCLUDE} -D PRIORITY
```

修改完成后，直接进行make all即可完成编译

### 4.2 测试

为方便演示测试结果，以下测试内容`NR_TASKS`均为5.

#### 4.2.1 SJF

```
Build Finished OK
Launch the qemu ......

OpenSBI v0.9
   ____                    _____ ____ _____
  / __ \                  / ____|  _ \_   _|
 | |  | |_ __   ___ _ __ | (___ | |_) || |
 | |  | | '_ \ / _ \ '_ \ \___ \|  _ < | |
 | |__| | |_) |  __/ | | |____) | |_) || |_
  \____/| .__/ \___|_| |_|_____/|____/_____|
        | |
        |_|

Platform Name             : riscv-virtio,qemu
Platform Features         : timer,mfdeleg
Platform HART Count       : 1
Firmware Base             : 0x80000000
Firmware Size             : 100 KB
Runtime SBI Version       : 0.2

Domain0 Name              : root
Domain0 Boot HART         : 0
Domain0 HARTs             : 0*
Domain0 Region00          : 0x0000000080000000-0x000000008001ffff ()
Domain0 Region01          : 0x0000000000000000-0xffffffffffffffff (R,W,X)
Domain0 Next Address      : 0x0000000080200000
Domain0 Next Arg1         : 0x0000000087000000
Domain0 Next Mode         : S-mode
Domain0 SysReset          : yes

Boot HART ID              : 0
Boot HART Domain          : root
Boot HART ISA             : rv64imafdcsu
Boot HART Features        : scounteren,mcounteren,time
Boot HART PMP Count       : 16
Boot HART PMP Granularity : 4
Boot HART PMP Address Bits: 54
Boot HART MHPM Count      : 0
Boot HART MHPM Count      : 0
Boot HART MIDELEG         : 0x0000000000000222
Boot HART MEDELEG         : 0x000000000000b109
...mm_init done!
...proc_init done!
2022 Hello RISC-V
kernel is running!
kernel is running!
[S] Supervisor Mode Timer Interrupt
SET [PID = 1 COUNTER = 10]
SET [PID = 2 COUNTER = 10]
SET [PID = 3 COUNTER = 5]
SET [PID = 4 COUNTER = 2]
pid=1 counter=10
pid=2 counter=10
pid=3 counter=5
pid=4 counter=2
switch to [PID = 4 COUNTER = 2]
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
pid=1 counter=10
pid=2 counter=10
pid=3 counter=5
pid=4 counter=0
switch to [PID = 3 COUNTER = 5]
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 2
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 3
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 4
[S] Supervisor Mode Timer Interrupt
pid=1 counter=10
pid=2 counter=10
pid=3 counter=0
pid=4 counter=0
switch to [PID = 1 COUNTER = 10]
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 2
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 3
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 4
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 5
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 6
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 7
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 8
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 9
[S] Supervisor Mode Timer Interrupt
pid=1 counter=0
pid=2 counter=10
pid=3 counter=0
pid=4 counter=0
switch to [PID = 2 COUNTER = 10]
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 2
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 3
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 4
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 5
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 6
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 7
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 8
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 9
[S] Supervisor Mode Timer Interrupt
SET [PID = 1 COUNTER = 9]
SET [PID = 2 COUNTER = 4]
SET [PID = 3 COUNTER = 4]
SET [PID = 4 COUNTER = 10]
pid=1 counter=9
pid=2 counter=4
pid=3 counter=4
pid=4 counter=10
switch to [PID = 2 COUNTER = 4]
[PID = 2] is running. auto_inc_local_var = 10
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 11
QEMU: Terminated
```

经检测，SJF调度中完成了线程切换与重新赋值，通过测试。

#### 4.2.2 PRIORITY

```
Build Finished OK
Launch the qemu ......

OpenSBI v0.9
   ____                    _____ ____ _____
  / __ \                  / ____|  _ \_   _|
 | |  | |_ __   ___ _ __ | (___ | |_) || |
 | |  | | '_ \ / _ \ '_ \ \___ \|  _ < | |
 | |__| | |_) |  __/ | | |____) | |_) || |_
  \____/| .__/ \___|_| |_|_____/|____/_____|
        | |
        |_|

Platform Name             : riscv-virtio,qemu
Platform Features         : timer,mfdeleg
Platform HART Count       : 1
Firmware Base             : 0x80000000
Firmware Size             : 100 KB
Runtime SBI Version       : 0.2

Domain0 Name              : root
Domain0 Boot HART         : 0
Domain0 HARTs             : 0*
Domain0 Region00          : 0x0000000080000000-0x000000008001ffff ()
Domain0 Region01          : 0x0000000000000000-0xffffffffffffffff (R,W,X)
Domain0 Next Address      : 0x0000000080200000
Domain0 Next Arg1         : 0x0000000087000000
Domain0 Next Mode         : S-mode
Domain0 SysReset          : yes

Boot HART ID              : 0
Boot HART Domain          : root
Boot HART ISA             : rv64imafdcsu
Boot HART Features        : scounteren,mcounteren,time
Boot HART PMP Count       : 16
Boot HART PMP Granularity : 4
Boot HART PMP Address Bits: 54
Boot HART MHPM Count      : 0
Boot HART MHPM Count      : 0
Boot HART MIDELEG         : 0x0000000000000222
Boot HART MEDELEG         : 0x000000000000b109
...mm_init done!
...proc_init done!
2022 Hello RISC-V
kernel is running!
kernel is running!
[S] Supervisor Mode Timer Interrupt
SET [PID = 1 PRIORITY = 10 COUNTER = 10]
SET [PID = 2 PRIORITY = 2 COUNTER = 5]
SET [PID = 3 PRIORITY = 4 COUNTER = 9]
SET [PID = 4 PRIORITY = 10 COUNTER = 4]
pid=1 PRIORITY = 10 counter=10
pid=2 PRIORITY = 2 counter=5
pid=3 PRIORITY = 4 counter=9
pid=4 PRIORITY = 10 counter=4
switch to [PID = 1 PRIORITY = 10 COUNTER = 10]
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 2
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 3
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 4
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 5
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 6
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 7
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 8
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 9
[S] Supervisor Mode Timer Interrupt
pid=1 PRIORITY = 10 counter=0
pid=2 PRIORITY = 2 counter=5
pid=3 PRIORITY = 4 counter=9
pid=4 PRIORITY = 10 counter=4
switch to [PID = 4 PRIORITY = 10 COUNTER = 4]
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 2
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 3
[S] Supervisor Mode Timer Interrupt
pid=1 PRIORITY = 10 counter=0
pid=2 PRIORITY = 2 counter=5
pid=3 PRIORITY = 4 counter=9
pid=4 PRIORITY = 10 counter=0
switch to [PID = 3 PRIORITY = 4 COUNTER = 9]
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 2
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 3
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 4
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 5
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 6
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 7
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 8
[S] Supervisor Mode Timer Interrupt
pid=1 PRIORITY = 10 counter=0
pid=2 PRIORITY = 2 counter=5
pid=3 PRIORITY = 4 counter=0
pid=4 PRIORITY = 10 counter=0
switch to [PID = 2 PRIORITY = 2 COUNTER = 5]
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 1
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 2
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 3
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 4
[S] Supervisor Mode Timer Interrupt
SET [PID = 1 PRIORITY = 10 COUNTER = 5]
SET [PID = 2 PRIORITY = 7 COUNTER = 4]
SET [PID = 3 PRIORITY = 8 COUNTER = 5]
SET [PID = 4 PRIORITY = 9 COUNTER = 8]
pid=1 PRIORITY = 10 counter=5
pid=2 PRIORITY = 7 counter=4
pid=3 PRIORITY = 8 counter=5
pid=4 PRIORITY = 9 counter=8
switch to [PID = 1 PRIORITY = 10 COUNTER = 5]
[PID = 1] is running. auto_inc_local_var = 10
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 11
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 12
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 13
[S] Supervisor Mode Timer Interrupt
[PID = 1] is running. auto_inc_local_var = 14
[S] Supervisor Mode Timer Interrupt
pid=1 PRIORITY = 10 counter=0
pid=2 PRIORITY = 7 counter=4
pid=3 PRIORITY = 8 counter=5
pid=4 PRIORITY = 9 counter=8
switch to [PID = 4 PRIORITY = 9 COUNTER = 8]
[PID = 4] is running. auto_inc_local_var = 4
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 5
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 6
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 7
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 8
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 9
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 10
[S] Supervisor Mode Timer Interrupt
[PID = 4] is running. auto_inc_local_var = 11
[S] Supervisor Mode Timer Interrupt
pid=1 PRIORITY = 10 counter=0
pid=2 PRIORITY = 7 counter=4
pid=3 PRIORITY = 8 counter=5
pid=4 PRIORITY = 9 counter=0
switch to [PID = 3 PRIORITY = 8 COUNTER = 5]
[PID = 3] is running. auto_inc_local_var = 9
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 10
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 11
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 12
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 13
[S] Supervisor Mode Timer Interrupt
pid=1 PRIORITY = 10 counter=0
pid=2 PRIORITY = 7 counter=4
pid=3 PRIORITY = 8 counter=0
pid=4 PRIORITY = 9 counter=0
switch to [PID = 2 PRIORITY = 7 COUNTER = 4]
[PID = 2] is running. auto_inc_local_var = 5
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 6
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 7
[S] Supervisor Mode Timer Interrupt
[PID = 2] is running. auto_inc_local_var = 8
[S] Supervisor Mode Timer Interrupt
SET [PID = 1 PRIORITY = 8 COUNTER = 6]
SET [PID = 2 PRIORITY = 3 COUNTER = 10]
SET [PID = 3 PRIORITY = 10 COUNTER = 8]
SET [PID = 4 PRIORITY = 3 COUNTER = 1]
pid=1 PRIORITY = 8 counter=6
pid=2 PRIORITY = 3 counter=10
pid=3 PRIORITY = 10 counter=8
pid=4 PRIORITY = 3 counter=1
switch to [PID = 3 PRIORITY = 10 COUNTER = 8]
[PID = 3] is running. auto_inc_local_var = 14
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 15
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 16
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 17
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 18
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 19
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 20
[S] Supervisor Mode Timer Interrupt
[PID = 3] is running. auto_inc_local_var = 21
QEMU: Terminated
```

经检测，PRIORITY调度中完成了线程切换与重新赋值，通过测试。

# 思考题

## 1

因为s0~s11是callee save的寄存器，因此需要在线程调度的时候储存。

而sp与ra都储存了独属于该线程的数据（栈指针和return address），因此也需要被储存。

其余的寄存器均为caller save寄存器，因此不需要由该线程单独储存。

## 2

在此之后每一次ra的返回点依旧是\_\_dummy。因为我们在线程初始化的时候将线程的ra赋上了\_\_dummy，而之后我们则是使用

我们来看从pid=12的线程切换到pid=28的线程，由于我们是在\_\_switch\_to的第一行就保存了ra，因此我们在\_\_switch\_to设一个断点，而我们可以通过寄存器的查看得知，此时的ra的值是调用\_\_switch\_to的后一行代码。

![image-20221027223530912](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221027223531.png)
