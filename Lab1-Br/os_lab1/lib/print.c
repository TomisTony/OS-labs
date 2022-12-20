#include "print.h"
#include "sbi.h"

void puts(char *s)
{
    char tmp;
    for (int i = 0;; i++)
    {
        if (s[i] != '\0')
            sbi_ecall(0x1, 0x0, s[i], 0, 0, 0, 0, 0);
        else
            break;
    }
}

void puti(int x)
{
    char a[10];
    int count = 0;
    while(x != 0){
        a[count++] = x % 10 + '0';
        x /= 10;
    }
    for(int i = count-1; i >=0; i--){
        struct sbiret ret = sbi_ecall(0x1, 0x0, a[i], 0, 0, 0, 0, 0);
    }
    
}
