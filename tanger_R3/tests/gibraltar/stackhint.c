#include <stdio.h>
#include <stdlib.h>

#include <tanger-stm.h>

int values[] = {40, 10, 100, 90, 20, 60, 50};

int cmp(const void *a, const void *b)
{
	return ( *(int*)a - *(int*)b );
}

void do_something()
{
	qsort(values, sizeof(values)/sizeof(int), sizeof(int), cmp);
}

int main(int argc, char **argv)
{
	int i;

	tanger_init();
	tanger_thread_init();
	tanger_begin();

	i = 0;
	i += 40;
	values[0] = i;
	
	do_something();

	tanger_commit();
	tanger_thread_shutdown();
	tanger_shutdown();
	
	for (i = 0; i < sizeof(values)/sizeof(int); i++)
		printf("%d\t", values[i]);
	printf("\n");
	
	return 0;
}
