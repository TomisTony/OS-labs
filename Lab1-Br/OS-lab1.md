# OS-lab1

## 连接并开启docker

![image-20220922194927000](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220922194934.png)

## 编写head.S

![image-20220925141036482](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925141043.png)

## 编写lib/Makefile

![image-20220922201813030](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220922201813.png)

## 补充sbi.c

![image-20220925141105638](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925141105.png)

## puts()和puti()

![image-20220925141126098](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925141126.png)

## 修改defs

![image-20220926001034860](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926001034.png)

# 思考题

## 1

- 变量应该尽量存放在寄存器中，且避免频繁恢复和保存寄存器
- 储存返回值的寄存器、传递参数的寄存器都可以不保留原值。其他寄存器需要保留原值。
- 如果参数和局部变量太多而无法在寄存器中存下，函数的开头会在栈中为函数帧分配空间来存放。当一个函数的功能完成后将会释放栈帧并返回调用点。

> Caller Saved Register字面意思是由调用方负责保存的寄存器，被调用方可以随意使用。
>
> Callee Saved Regiser字面意思是由被调用法负责保存的寄存器，在调用结束时，被调用方需要恢复被调用前的寄存器的值。

## 2

![image-20220925141247437](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925141247.png)

## 3

修改了main.c使得我们可以读取sstatus

![image-20220925145956079](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925145956.png)

重新编译并开启debug，使用gdb查看，最终的读取结果是：

![image-20220925155646729](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925155646.png)

说明读取成功。

查询资料后可以得知：

SD 位是一个只读位，被置1，说明了 FS、VS 或 XS 字段存在一些需要将扩展用户上下文保存到内存的脏状态。

而FS域描述浮点数单元状态，被置11，说明浮点数单元设置为dirty状态。全局关闭中断模式。

## 4

修改main.c为：

![image-20220925193048737](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925193048.png)

开启gdb进行调试，发现csrw成功修改sscratch的值

![image-20220925193000511](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220925193000.png)

## 5

下载并解压linux-6.0源码

![image-20220926164351067](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926164358.png)

选择arm64架构

![image-20220926164444304](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926164444.png)

开始用arm64架构交叉编译

![image-20220926185911874](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926185911.png)

编译完成后我们可以看到我们成功获得了sys.i

![image-20220926190032963](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926190033.png)

## 6

### ARM32，x86(32 bit)，x86_64

我们使用find命令在文件夹里面寻找，可以发现ARM32，x86(32 bit)，x86_64的syscall_table已经存在了。

![image-20220928160245148](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220928160245.png)

接下来我们需要的就是RISC-V的syscall_table

### RISC-V(64 bit)

我们使用64位编译配置，使用交叉编译编译出arch/riscv/kernel/syscall_table.i

![image-20220928154616625](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220928154616.png)

![image-20220928154756616](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220928154756.png)

可以看到syscall_table就在该文件中。

![image-20220928160754329](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220928160754.png)



### RISC-V(32 bit) 

我们使用32位编译配置。

![image-20220928170134216](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220928170134.png)

使用交叉编译工具进行编译。

![image-20220928170900953](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220928170901.png)

成功得到32位的syscall_table

![image-20220928171249582](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220928171249.png)

## 7

> ELF文件是一种用于二进制文件、可执行文件、目标代码、共享库和核心转储格式文件的文件格式。

我们以本次lab的head.S编译出的head.o文件为例

![image-20220926122222854](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926122222.png)

使用readelf命令：

![image-20220926122310509](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926122310.png)

![image-20220926122358622](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926122358.png)

![image-20220926122413275](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926122413.png)

使用objdump命令：

![image-20220926130436104](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926130436.png)

由于可打印选项太多，我们以objdump -f为例

![image-20220926130505432](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926130505.png)

为了演示一个可以直接run的elf文件，我们以我之前写的一个奇怪的c程序为例：

![image-20220926131113998](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926131114.png)

查询该程序的PID

![image-20220926132505669](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926132505.png)

使用cat命令成功获取该程序的memory layout

![image-20220926132600551](https://br-1313886514.cos.ap-shanghai.myqcloud.com/20220926132600.png)