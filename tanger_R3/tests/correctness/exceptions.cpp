#include <stdio.h>

int main(int argc, const char** argv)
{
    try {
        __transaction {
            if (argc == 0) throw (int)1;
            __transaction {
                if (argc == 1) throw (const char*)"bar";
            }
        }
    }
    catch (int x) {
        printf("%d\n", x);
    }
    catch (const char* s) {
        printf("%s\n", s);
    }
}
