# OS lab4

## 1. 准备工程

按照要求在`defs.h`中添加以下宏定义：



![image-20221202202617585](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221202202617585.png)

将`vmlinux.lds.S`, `Makefile`放在正确的地方
![image-20221202202722260](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221202202722260.png)

## 2. 开启虚拟内存映射

### 2.1 setup_vm

首先，我们需要将 0x80000000 开始的 1GB 区域进行一次是等值映射 ( PA == VA ) ，和一次将其映射至高地址 ( PA + PV2VA_OFFSET == VA )

```c++
void setup_vm(void)
{
    unsigned long true_early_pgtbl = (unsigned long)early_pgtbl - PA2VA_OFFSET;
    memset((void *)true_early_pgtbl, 0, PGSIZE);
    ((unsigned long *)true_early_pgtbl)[getVPN(PHY_START, 2)] = (0xf) | ((getPPN(PHY_START, 2)) << 28);
    ((unsigned long *)true_early_pgtbl)[getVPN(VM_START, 2)] = (0xf) | ((getPPN(PHY_START, 2)) << 28);
    printk("setup_vm done!\n");
}
```

由于reallocate后均使用虚拟地址，所以我们在`setup_vm`中必须要对原本的给定的页表地址减去地址偏移，使其变为`early_pgtbl`真正的物理地址，然后我们再为真正的页表分配1GB空。

这之后，我们根据提示，将va，pa的30~38位取出分别作为`early_pgtbl`的index。

而对于页表项，我们要首先将0~3位置为1，代表将Page Table Entry 的权限 V | R | W | X 开启。然后将va,pa的的28~53位取出，左移28位作为PTE的PPN[2]，其余的位均置为0.![image-20221202204832031](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221202204832031.png)

完成了`setup_vm`函数的实现后，我们需要通过 `relocate` 函数，完成对 `satp` 的设置，以及跳转到对应的虚拟地址。

在reallocate中，我们首先需要将`ra`，`sp`寄存器加上一个偏移量，使其变为虚拟地址。

```assembly
li t0, 0xffffffe000000000
li t1, 0x0000000080000000
sub t0, t0, t1
add ra, ra, t0
add sp, sp, t0
```

然后我们需要将`satp`设置为`early_pgtbl`,注意t1一开始用la赋值时`early_pgtbl`是虚拟地址，需要减去一个偏移量来得到对应的物理地址再右移12位得到PPN。然后将8左移60位与之前所得相加，写入`satp`。最后刷新 TLB，刷新 icache。

![](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221202211613544.png)

```assembly
	# PPN
    la t1, early_pgtbl
    sub t1,t1,t0
    srli t1, t1, 12
    # MODE
    li t2,8
    slli t2, t2, 60
    # set satp
    add t1,t1,t2
    csrw satp,t1
    
    # flush tlb
    sfence.vma zero, zero

    # flush icache
    fence.i
```

### 2.2 `setup_vm_final` 的实现

首先修改`mm_init`，由于reallocate后我们全部使用逻辑地址，所以要为PHY_END`加上一个偏移量使其变为逻辑地址。

![image-20221202214840304](https://suonan-image.oss-cn-hangzhou.aliyuncs.com/img/image-20221202214840304.png)

我们首先来讲一下这部分的核心函数`create_mapping`, 用它来创建映射。

