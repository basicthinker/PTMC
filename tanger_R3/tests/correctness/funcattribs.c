#include <stdio.h>

//#include <string.h>
//void * memset(void *src, int c, size_t n);
//void * memset(void *src, int c, size_t n) __attribute__((tm_wrapper("tanger_stm_std_memset")));

/* checks have to be made for declarations only and declarations with definitions */

void test_pure() __attribute__((transaction_pure));
void test_pure() { asm volatile (""); }

void test_pure2(void) __attribute__((transaction_pure));

__attribute__((transaction_pure)) void test_pure3(void) { asm volatile (""); }

void test_callable() __attribute__((transaction_callable));
void test_callable() { asm volatile (""); }

void test_callable2() __attribute__((transaction_callable));

__attribute__((transaction_callable)) void test_callable3(void) { asm volatile (""); }

void test_wrap_orig(void) __attribute__((transaction_wrap("test_wrap_wrapped")));
void test_wrap_orig(void) { printf("not wrapped\n"); }
__attribute__((used)) void test_wrap_wrapped(void) { printf("wrapped\n"); }

void test_wrap_orig2(void) __attribute__((transaction_wrap("test_wrap_wrapped2")));
__attribute__((used)) void test_wrap_orig2(void) { printf("not wrapped\n"); }

void test_wrap_orig3(void) __attribute__((transaction_wrap("test_wrap_wrapped3")));
__attribute__((used)) void test_wrap_wrapped3(void) { printf("wrapped\n"); }

void test_wrap_orig4(void) __attribute__((transaction_wrap("test_wrap_wrapped4")));


/* TODO add tests for re-definitions */

int main(int argc, char** argv)
{
    __transaction {
{
int i;
}
        test_pure();
        //    test_pure2();
        test_pure3();

        test_callable();
        //    test_callable2();
        test_callable3();

        test_wrap_orig();
//        test_wrap_orig2();
//        test_wrap_orig3();
//        test_wrap_orig4();
    }

    return 0;
}
