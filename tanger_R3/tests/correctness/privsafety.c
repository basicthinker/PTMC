struct timespec;
int nanosleepfake(const struct timespec *req, struct timespec *rem) __attribute__((transaction_pure));
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned max = 5000000;
typedef struct {
    unsigned txnal;
    unsigned padding[124]; // try to put txnal and nontxnal into different locks
    unsigned nontxnal;
} Data;
Data data;

// count
void* updater(void* d)
{
    unsigned i;
    for (i = 1; i <= max; i++) {
        __transaction { data.txnal = i; }
        data.nontxnal = i;
    }
    return 0;
}

int nanosleepfake(const struct timespec *req, struct timespec *rem)
{
    return nanosleep(req, rem);
}

void error() __attribute__((transaction_pure));
void error(unsigned txnal, unsigned nontxnal)
{
    printf("txnal=%u nontxnal=%u\nFAILED\n", txnal, nontxnal);
    exit(EXIT_FAILURE);
}

void inc(const char* s, volatile unsigned* x) __attribute__((transaction_pure,noinline));
void inc(const char* s, volatile unsigned* x) { *x += 1; printf("%s inc %p\n", s, x);}

// if on stack, they will be rolled back (no precise stack rollback in DTMC yet)
static unsigned observations = 0, observationsFull = 0, observationsPre = 0;

int main(int argc, const char** argv)
{
    data.txnal = 0;
    data.nontxnal = 0;
    // prevents codegen bug (?, padding in Data is lost) in llc
    printf("txnal=%p nontxnal=%p\n", &data.txnal, &data.nontxnal);
    pthread_t ut;
    if (pthread_create(&ut, NULL, updater, NULL) != 0) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }
    // We must not see the nontxnal update after we not reading the matching txnal
    // update because this would violate privatization safety.
    unsigned rTxnal, rNontxnal;
    struct timespec timeout;
    timeout.tv_sec = 1;
    timeout.tv_nsec = 100000;
    do {
        __transaction {
            rTxnal = data.txnal;
            nanosleepfake(&timeout, NULL);
            inc("o", &observationsPre);
            rNontxnal = data.nontxnal;
            inc("f", &observationsFull);
	    //            if (rTxnal < rNontxnal) error(rTxnal, rNontxnal);
        }
        observations++;
    } while (rTxnal < max);
    printf("observations=%u (%u,%u,%u)\nSUCCESS\n", observations, observationsPre, observationsFull);
    return 0;
}
