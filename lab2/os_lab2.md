# OS_lab2

# 4.1 准备工程

成功同步代码并修改引用

![image-20221007183200741](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007183207.png)

成功修改vmlinux.lds文件

![image-20221007183511815](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007183511.png)

成功修改head.S

![image-20221007183611989](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007183612.png)

# 4.2 开启 trap 处理

首先设置sie以及sstatus

![image-20221007191829860](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007191829.png)

经过gdb检查，设置无误

![image-20221007192020141](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007192020.png)

设置stvec为_traps的地址，并设置第一次时钟中断为1s后。

![image-20221007222342129](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007222342.png)

# 4.3 实现上下文切换

首先编写entry.S,压栈保存各个寄存器的值并在trap结束之后重新读出

![image-20221007195629809](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007195629.png)

经过gdb调试，寄存器的存储和恢复是正常的

再将scause,sepc作为参数，调用trap_handler

![image-20221007222440316](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007222440.png)

最后使用sret从trap中返回

![image-20221017142452733](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221017142452733.png)

# 4.4 实现 trap 处理函数

编写trap.c文件，其中的`interrupt`与`timer`变量是用来判断`scause`对应的位上是否为1，从而判断此次trap是否为interrupt, 是否为timer interrupt。判断完成后，调用clock_set_next_event来设置下一次中断。

![image-20221007222133877](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007222133.png)

# 4.5 实现时钟中断相关函数

编写clock.c文件:

通过内联汇编来完成`get_cycles`函数，读出time寄存器中的值。并在`clock_set_next_event`中使用该值调用sbi_ecall来完成设置时钟中断。

![image-20221007222233904](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007222233.png)

# 4.6 编译及测试

经过测试，程序正常运行——每1s会打印一次中断信息。

![image-20221007222605384](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20221007222605.png)

# 思考题

```
Boot HART MIDELEG         : 0x0000000000000222
Boot HART MEDELEG         : 0x000000000000b109
```

![image-20221007224751127](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221007224751127.png)

MIDELEG: 第1, 5, 9位被置为1, 代表supervisor software interrupt, supervisor timer interrupt, supervisor external interrupt被代理给了S mode, 即当以上三种interrupt在S mode中发生时，会在S  mode中被执行。

MEDELEG: 第0, 3, 8, 12, 13, 15位被置为1, 对应Instruction address misaligned, Breakpoint, Environment call from U-mode, Instruction page fault, Load page fault, Store page fault, 这几种同步异常被M mode委托给了S mode, 即当以上几种异常在S mode中发生时，会在S  mode中被执行。

