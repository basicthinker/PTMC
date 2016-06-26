#include <stdio.h>

void test(int* x) __attribute__((noinline));
void test(int* x)
{
    __transaction {
        (*x)++;
        __transaction {
            (*x)++;
	    //            printf("%p", x);
        }
    }
}

void test2(int* x) __attribute__((noinline));
void test2(int* x)
{
    __transaction {
        (*x)++;
        __transaction {
            test(x);
        }
    }
}


int main(int argc, char** argv)
{
    int x;
	test2(&x);
	return x;
}
 
