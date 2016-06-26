#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "tanger-stm-std-math.h"
#include "tanger-stm-std-string.h"

char * p_c;

int compare (const void * a, const void * b)
{
    return ( *(char*)a - *(char*)b );
}

static void test_math() __attribute__((noinline));
static void test_math()
{
    volatile double result = 42;
    __transaction {
        result = log(result);
    }
}

static void test_mem() __attribute__((noinline));
static void test_mem()
{
    char src[42];
    char dest[42];
    char dest2[2*42];
    char c1, c2;
    int i;

    volatile size_t result = 42;

    // test plain copy
    memset(src, result, result);
    memset(dest, 0, result);

    __transaction {
      memcpy(dest, src, result);
    }
    
    for (i=0; i < result; ++i)
       assert(dest[i] == result);


    // test plain copy with update in copied region
    memset(dest, 0, result);
    __transaction {
      memcpy(dest, src, result);
      dest[21] = 5;
    }
    for (i=0; i < result; ++i)
      if(i != 21)
       assert(dest[i] == result);
    assert(dest[21] == 5);
    
    
    // test
    for (i = 0; i < result; i++)
      src[i] = i;

    memset(dest, 0, result);
    c1 = 0;
    c2 = 0;
    __transaction {
      memcpy(dest, src, result);
      dest[21] = 5;
      c1 = dest[21];
      memcpy(src, dest, result);
      c2 = src[21];
    }

    for (i = 0; i < result; i++) 
     if (i != 21) {
      assert(src[i] == i);
      assert(dest[i] == i);
    }

    assert(dest[21] == 5);
    assert(src[21] == 5);
    assert(c1 == 5);
    assert(c2 == 5);

    // test
    memset(src, 0, result);
    __transaction {
      memcpy(dest, src, result);
      for (i = 0; i < result; i++)
        dest[i] = i;
      memcpy(src, dest, result);
    }
    
    for (i = 0; i < result; i++) {
      assert(src[i] == i);
      assert(dest[i] == i);
    }

    // test different destinations with same source
    for (i = 0; i < result; i++)
      src[i] = i;
    memset(dest, 0, result);
    memset(dest2, 0, result*2);

    __transaction {
      src[5] = 7;
      p_c = dest;
      memcpy(p_c, src, result);
      src[5] = 8;
      p_c = dest2;
      memcpy(p_c, src, result);
    }

    for (i = 0; i < result; i++)
      if ( i != 5) {
	assert(dest[i] == i);
	assert(dest2[i] == i);
	assert(src[i] == i);
      }
 
    assert(src[5] == 8);
    assert(dest[5] == 7);
    assert(dest2[5] == 8);

    
    memset(src, 42, result);
    memset(dest, 0, result);
    memset(dest2, 0, result*2);

    __transaction {
      src[5] = 7;
      memcpy(dest, src, result);
      dest[6] = 8;
      memcpy(dest2, dest, result);
    }

    assert(dest[6] == 8);
    assert(dest[1] == 42);
    assert(dest[5] == 7);
    
    assert(dest2[6] == 8);
    assert(dest2[1] == 42);
    assert(dest2[5] == 7);

    
    // test: new region ends in old region and begins before
    for(i=0; i < result; ++i)
      src[i] = i;
    memset(dest, result, result);
     
    __transaction {
       src[15] = 2;
       memcpy(&dest[10], &src[10], 10);
       src[7] = 4;
       memcpy(&dest[5], &src[5], 10);
     }
    for (i=0; i < 5; ++i)
      assert(dest[i] == result);
    for (i=5; i < 20; ++i)
      if (i != 7 && i != 15)
	assert(dest[i] == i);
    for (i=20; i < result; ++i)
      assert(dest[i] == result);
     
    assert(dest[7] == 4);
    assert(dest[15] == 2);

    // test: new region starts in old region but is larger
    memset(src, 0, result);
    memset(dest, result, result);

    for (i=0; i < result; ++i)
       src[i] = i;

    __transaction {
       src[7] = 4;
       memcpy(&dest[5], &src[5], 10);
       src[7] = 8;
       src[15] = 2;
       memcpy(&dest[10], &src[10], 10);
     }
    
//    for (i=0; i < result; ++i)
//      printf("%u %u:%u\n", i, src[i], dest[i]);

    for (i=0; i < 5; ++i)
      assert(dest[i] == result);
    for (i=5; i < 20; ++i)
      if (i != 7 && i != 15)
	assert(dest[i] == i);
    for (i=20; i < result; ++i)
      assert(dest[i] == result);
    assert(dest[7] == 4);
    assert(dest[15] == 2);

    // test: new region includes old region completely
    for(i=0; i < result; ++i)
      src[i] = i;


    memset(dest, result, result);
    __transaction {
       src[15] = 2;
       memcpy(&dest[10], &src[10], 10);
       src[7] = 4;
       memcpy(&dest[5], &src[5], 20);
     }
    for (i=0; i < 5; ++i)
      assert(dest[i] == result);
    for (i=5; i < 25; ++i)
      if (i != 7 && i != 15)
	assert(dest[i] == i);
    for (i=25; i < result; ++i)
      assert(dest[i] == result);
    assert(dest[7] == 4);
    assert(dest[15] == 2);

    // another test
    for(i=0; i < result; ++i)
      src[i] = i;
    memset(dest, result, result);
     __transaction {
       src[20] = 4;
       memcpy(&dest[5], &src[5], result - 15);
       src[1] = 9;
       memcpy(dest, src, 6);
       dest[40] = 8;
       memset(src, 0, result);
       memcpy(src, dest, result);
     }

    assert(src[1] == 9);
    assert(src[20] == 4);
    assert(src[40] == 8);
    assert(src[0] == 0);
    assert(src[result-1] == result);

    // test merge cases
    memset(src, 0, result);
    memset(dest, result, result);
    memset(dest2, 0, result*2);
    for(i=0; i < result; ++i)
      src[i] = i;
    __transaction {
       src[12] = 2;
       memcpy(&dest[10], &src[10], 5);
       src[22] = 4;
       memcpy(&dest[20], &src[20], 5);
       src[32] = 8;
       memcpy(&dest[30], &src[30], 5);
       dest[40] = 16;

       memcpy(dest2, dest, result);
       memcpy(dest, src, result);
     }
     
    assert(dest[12] == 2);
    assert(dest[22] == 4);
    assert(dest[32] == 8);
     
    assert(dest2[12] == 2);
    assert(dest2[22] == 4);
    assert(dest2[32] == 8);
    assert(dest2[40] == 16);

    memset(src, 0, result);
    memset(dest, 0, result);
    __transaction {
        memset(src, result, result);
        memcpy(dest, src, result);
        memmove(src, dest, result);
    }

    memset(src, 0, result);
    memset(dest, result, result);
    memset(dest2, result*2, result*2);

    __transaction {
       memcpy(&dest[10], &src[10], 5);
      for (i=0; i < result; ++i)
	src[i] = i;
       memcpy(&dest[20], &src[20], 5);

     }
     
    for (i=0; i < result; ++i) {
      assert(src[i] == i);
    }
      
    
    
}

int main(int argc, const char** argv)
{
    test_math();
#ifndef __APPLE__
    test_mem();
#endif
    return 0;
}
