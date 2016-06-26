volatile int* dummy;
int* dummy2;

int notshared(int l) __attribute__((noinline));
int notshared(int l)
{
    int a = l*l;
    int x;
    __transaction {
        x = a - l;
        *dummy2 = x;
        __asm__ __volatile__("");
    }
    return x;
}

int notshared2(int l) __attribute__((noinline));
int notshared2(int l)
{
    int a = l*l;
    int x;
    dummy = &x;
    __transaction {
        a = a - l;
        *dummy2 = a;
        __asm__ __volatile__("");
    }
    x = a*a*a;
    return x;
}

int shared(int l) __attribute__((noinline));
int shared(int l)
{
    int a = l*l;
    int x;
    dummy = &x;
    __transaction {
        *dummy2 = a;
        for (; l > 0; l--) x *= a;
        __asm__ __volatile__("");
    }
    return x;
}

int main()
{
    int x;
    dummy = &x;
    dummy2 = &x;
    x = notshared(1);
    x = notshared(x);
    x = shared(x);
}
