#include "defs.h"
#include "string.h"
#include "mm.h"

#include "printk.h"

extern char _ekernel[];

struct {
    struct run *freelist;
} kmem;

uint64 kalloc() {
    struct run *r;

    r = kmem.freelist;
    kmem.freelist = r->next;
    
    memset((void *)r, 0x0, PGSIZE);
    return (uint64) r;
}

void kfree(uint64 addr) {
    struct run *r;

    // PGSIZE align 
    addr = addr & ~(PGSIZE - 1);

    memset((void *)addr, 0x0, (uint64)PGSIZE);

    r = (struct run *)addr;
    r->next = kmem.freelist;
    kmem.freelist = r;

    return ;
}

void kfreerange(uint64 start, uint64 end) {
    uint64 addr = (uint64)PGROUNDUP((uint64)start);
    for (; addr + PGSIZE <= (uint64)end; addr += PGSIZE) {
        kfree((uint64)addr);
    }
}

void mm_init(void) {
    //TODO: kfreerange scause = 0x7
    kfreerange((uint64)_ekernel, (uint64)(VM_START+PHY_SIZE));
    printk("...mm_init done!\n");
}
