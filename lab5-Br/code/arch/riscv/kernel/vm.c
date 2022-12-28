#include "defs.h"
#include "mm.h"
#include "string.h"

typedef unsigned long uint64;

extern char _stext[];
extern char _srodata[];
extern char _sdata[];

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));

unsigned long getVPN(unsigned long va, int n)
{
    return (va >> (12 + n * 9)) & 0x1ff;
}
unsigned long getPPN(unsigned long pa, int n)
{
    if (n == 2)
        return (pa >> 30) & 0x7fff;
    else
        return (pa >> (12 + n * 9)) & 0x1ff;
}

uint64 getPPNFromPA(uint64 addr){
    return (addr >> 12) & 0xfffffffffff;
}
uint64 getPPNFromEntry(uint64 addr){
    return (addr >> 10) & 0xfffffffffff;
}

void setup_vm(void)
{
    /*
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    unsigned long true_early_pgtbl = (unsigned long)early_pgtbl - PA2VA_OFFSET;
    memset((void *)true_early_pgtbl, 0, PGSIZE);
    ((unsigned long *)true_early_pgtbl)[getVPN(PHY_START, 2)] = (0xf) | ((getPPN(PHY_START, 2)) << 28);
    ((unsigned long *)true_early_pgtbl)[getVPN(VM_START, 2)] = (0xf) | ((getPPN(PHY_START, 2)) << 28);
    // memset(early_pgtbl, 0, PGSIZE);
    // early_pgtbl[getVPN(PHY_START, 2)] = (0xf) | (getVPN(PHY_START, 2)) << 28;
    // early_pgtbl[getVPN(VM_START, 2)] = (0xf) | (getVPN(PHY_START, 2)) << 28;
    printk("setup_vm done!\n");
}


/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long  swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    printk("setup_vm_final begin!\n");

    memset(swapper_pg_dir, 0x0, PGSIZE);
    printk("pgdir memset done!\n");

    // No OpenSBI mapping required

    uint64 virtualAddress = VM_START + OPENSBI_SIZE;
    uint64 physicalAddress = PHY_START + OPENSBI_SIZE;
    uint64 kernalTextSize = (uint64)_srodata - (uint64)_stext;
    uint64 kernalRodataSize = (uint64)_sdata - (uint64)_srodata;
    uint64 otherMemorySize = (uint64)(PHY_SIZE) - ((uint64)_sdata - (uint64)_stext);

    printk("mapping begin!\n");
    // mapping kernel text X|-|R|V
    create_mapping(swapper_pg_dir, virtualAddress, physicalAddress, kernalTextSize, 11);

    // mapping kernel rodata -|-|R|V
    virtualAddress += kernalTextSize;
    physicalAddress += kernalTextSize;
    create_mapping(swapper_pg_dir, virtualAddress, physicalAddress, kernalRodataSize, 3);

    // mapping other memory -|W|R|V
    virtualAddress += kernalRodataSize;
    physicalAddress += kernalRodataSize;
    create_mapping(swapper_pg_dir, virtualAddress, physicalAddress, otherMemorySize, 7);

    printk("mapping done!\n");
    // set satp with swapper_pg_dir
    //使用宏定义
    uint64 phyPGDir = (uint64)swapper_pg_dir - PA2VA_OFFSET;
    asm volatile ( 
        "mv t1, %[phyPGDir]\n"
        "srli t1, t1, 12\n"
        "li t0, 8\n"
        "slli t0, t0, 60\n"
        "add t0, t0, t1\n"
        "csrw satp, t0"
        :
        :[phyPGDir]"r"(phyPGDir)
        :"memory"
    );

    printk("satp set done!\n");

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    return;
}

/* 创建多级页表映射关系 */
/*
    pgtbl 为根页表的基地址
    virtualAddress, physicalAddress 为需要映射的虚拟地址、物理地址,后offset均为0(因为有4K对齐)
    size 为映射的大小,应为4kb的倍数(大概)
    perm 为映射的读写权限
    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
*/
void create_mapping(uint64 *pgtbl, uint64 virtualAddress, uint64 physicalAddress, uint64 size, int perm) {
    int page_num = ((size-1) / (uint64)PGSIZE)+1; //一共需要映射的页数,整数除法，向上取整
    int pgtblIndex = getVPN(virtualAddress,2);//目前已分配到的根页表Entry的Index
    int secondLevelPageTableEntryIndex = getVPN(virtualAddress,1);
    int thirdLevelPageTableEntryIndex = getVPN(virtualAddress,0);
    uint64* secondEntryAddress;//根页表的Entry中储存的PPN，也就是我们要的二级页表地址
    uint64* thirdEntryAddress;//二级页表的Entry中储存的PPN，也就是我们要的三级页表地址

    for(int i = 0; i < page_num; i++){

        //step1: 获取根页表存的二级页表的Entry的地址
        uint64* nowPgtblEntryAddress = pgtbl + pgtblIndex;//pgtbl的某个Entry(储存我们要的二级页表)的地址

        //step2: 在根页表上找到对应的二级页表的地址，对应的Entry没有内容的话就new一个二级页表存到Entry
        //检查是否在一个空Entry上
        if(isPageNotValid(nowPgtblEntryAddress)){
            //new一个二级页表来填充这块区域
            secondEntryAddress = (uint64*)kalloc();
            *nowPgtblEntryAddress = (getPPNFromPA((uint64)secondEntryAddress - PA2VA_OFFSET) << 10) + 0x1;
        }
        else{
            secondEntryAddress = (getPPNFromEntry(*nowPgtblEntryAddress) << 12) + PA2VA_OFFSET;
        }

        //step3: 获取二级页表的存三级页表的Entry的地址

        //二级页表的某个Entry(储存我们要的三级页表)的地址
        uint64* nowSecondEntryAddress = secondEntryAddress + secondLevelPageTableEntryIndex;

        //step4: 在二级页表上找到对应的三级页表的地址，对应的Entry没有内容的话就new一个三级页表存到Entry

        //检查是否在一个空Entry上
        if(isPageNotValid(nowSecondEntryAddress)){
            //new一个三级页表来填充这块区域
            thirdEntryAddress = (uint64*)kalloc();
            *nowSecondEntryAddress = (getPPNFromPA((uint64)thirdEntryAddress - PA2VA_OFFSET) << 10) + 0x1;
        }
        else{
            thirdEntryAddress = ((getPPNFromEntry(*nowSecondEntryAddress)) << 12) + PA2VA_OFFSET;
        }

        //step5: 获取三级页表的存真实物理地址的Entry的地址
        uint64* nowPhysicalEntryAddress = thirdEntryAddress + thirdLevelPageTableEntryIndex;

        //step6: 写入三级页表(这个Entry一定是一个空内容)
        uint64 nowPhysicalAddress = i * PGSIZE + physicalAddress;
        *nowPhysicalEntryAddress = (getPPNFromPA(nowPhysicalAddress) << 10) + perm;
    
        //step7: 增加并更新各个页表的index
        thirdLevelPageTableEntryIndex++;

        //如果装满了二级页表和三级页表，则需要更换根页表的Entry(即+1)
        if(thirdLevelPageTableEntryIndex == 512){
            thirdLevelPageTableEntryIndex = 0;
            secondLevelPageTableEntryIndex++;
        }
        if(secondLevelPageTableEntryIndex == 512){
            secondLevelPageTableEntryIndex = 0;
            pgtblIndex++;
        }
        // if(pgtbl == 512){/*error*/}

    }
}


int isPageNotValid(uint64* addr){
    return !((*addr) & 0x1);
}
