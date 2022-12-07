target remote :1234
layout asm
b *0x80200024
b *0x8020011c
n
b *0xffffffe00020002c


