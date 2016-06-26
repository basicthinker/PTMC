#include <stdio.h>
#include <stdlib.h>

#include <tanger-stm.h>

int cmp(const void *a, const void *b)
{
	return ( *(int*)a - *(int*)b );
}

int main(int argc, char **argv)
{
	int values1[] = {40, 10, 100, 90, 20, 60, 50};
	int values2[] = {40, 10, 100, 90, 20, 60, 50};
	int i;

	qsort(values1, sizeof(values1)/sizeof(int), sizeof(int), cmp);

	tanger_init();
	tanger_thread_init();
	tanger_begin();

	i = 0;
	i += 40;
	values2[0] = i;

	qsort(values2, sizeof(values2)/sizeof(int), sizeof(int), cmp);

	tanger_commit();
	tanger_thread_shutdown();
	tanger_shutdown();

	for (i = 0; i < sizeof(values2)/sizeof(int); i++)
		printf("%d\t", values2[i]);
	printf("\n");

	return 0;
}
