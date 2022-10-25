#include "printk.h"
#include "defs.h"

// Please do not modify

void test() {
    long long count = 0;
    while (1){
        if(count++==100000000){
            printk("kernel is running!\n");
            count=0;
        }
    }
}
