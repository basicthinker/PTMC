#include <stdio.h>

int main(int argc, const char** argv)
{
    __transaction {
        asm volatile ("nop");
    }
    __transaction {
        printf("success\n");
    }
}
