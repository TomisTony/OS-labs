#include "print.h"
#include "sbi.h"
#include "defs.h"

extern void test();

int start_kernel() {
    puti(2022);
    puts(" Hello RISC-V\n");
    csr_read(0x100);

    test(); // DO NOT DELETE !!!

	return 0;
}
