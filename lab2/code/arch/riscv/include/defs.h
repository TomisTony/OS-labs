#ifndef _DEFS_H
#define _DEFS_H

#include "types.h"

#define csr_read(csr)                          \
    ({                                         \
        register uint64 __v;                   \
        asm volatile("csrr %[__v] ," #csr "\n" \
                     : [__v] "=r"(__v)         \
                     :                         \
                     : "memory");              \
        __v;                                   \
    })

#define csr_write(csr, val)              \
    ({                                   \
        uint64 __v = (uint64)(val);      \
        asm volatile("csrw " #csr ", %0" \
                     :                   \
                     : "r"(__v)          \
                     : "memory");        \
    })

#endif
