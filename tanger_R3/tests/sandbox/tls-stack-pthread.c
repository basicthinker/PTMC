/* Copyright (C) 2007  Torvald Riegel, Pascal Felber
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

volatile x;
__thread volatile xtls;

__thread int *sum;
pthread_key_t pk;
int *ssum;

int stls(int recur, int arg2, int arg3, int *sum)
{
	if (recur <= 0) return *sum + *sum*2;
	else {
		int r = stls(recur-1, (arg2 * 5) % 23, (arg3 * 13 + 23) % 17, sum);
		*sum += 1;
		return (*sum + r) % 21;
	} 
}

int tls(int recur, int arg2, int arg3)
{
	if (recur <= 0) return *sum + *sum*2;
	else {
		int r = tls(recur-1, (arg2 * 5) % 23, (arg3 * 13 + 23) % 17);
		*sum += 1;
		return (*sum + r) % 21;
	} 
}


int ptls(int recur, int arg2, int arg3)
{
	if (recur <= 0) {
		int *s = (int*)pthread_getspecific(pk);
		return *s + *s*2;
	}
	else {
		int r = ptls(recur-1, (arg2 * 5) % 23, (arg3 * 13 + 23) % 17);
		int *s = (int*)pthread_getspecific(pk);
		*s += 1;
		pthread_setspecific(pk, s);
		return (*s + r) % 21;
	} 
}

double timediff(struct timeval *start, struct timeval *end)
{
	return (double)end->tv_sec - start->tv_sec + (double)(end->tv_usec - start->tv_usec) / 1000000;
}

int main(int argc, char **argv)
{
	int iter = 100000000;
	int i;
	struct timeval start;
	struct timeval end;
	pthread_key_create(&pk, NULL);
	int s;

	gettimeofday(&start, 0);
	int y= 1;
	for (i = 0; i < iter; i++) {
		y += (y * 33) % 13;
	}
	gettimeofday(&end, 0);
	printf("y=%d\n", y);
	printf("reg time=%lf\n", timediff(&start, &end));  
	gettimeofday(&start, 0);
	x= 1;
	for (i = 0; i < iter; i++) {
		x += (x * 33) % 13;
	}
	gettimeofday(&end, 0);
	printf("x=%d\n", x);
	printf("volatile time=%lf\n", timediff(&start, &end));  
	gettimeofday(&start, 0);
	xtls= 1;
	for (i = 0; i < iter; i++) {
		xtls += (xtls * 33) % 13;
	}
	gettimeofday(&end, 0);
	printf("x=%d\n", x);
	printf("tls time=%lf\n", timediff(&start, &end));  


	// pass ctx as arg
	gettimeofday(&start, 0);
	s = 1;
	for (i = 0; i < iter; i++) {
		s = s + stls((i+18) % 13, i % 7, i % 19, &s);
	}
	gettimeofday(&end, 0);
	printf("sum=%d\n", s);
	printf("stack time=%lf\n", timediff(&start, &end));  
	// ctx in TLS
	gettimeofday(&start, 0);
	s = 1;
	sum = &s;
	for (i = 0; i < iter; i++) {
		s = s + tls((i+18) % 13, i % 7, i % 19);
	}
	gettimeofday(&end, 0);
	printf("sum=%d\n", s);
	printf("tls time=%lf\n", timediff(&start, &end));  
	// ctx in pthread TLS
	gettimeofday(&start, 0);
	s = 1;
	pthread_setspecific(pk, &s);
	for (i = 0; i < iter; i++) {
		s = s + tls((i+18) % 13, i % 7, i % 19);
	}
	gettimeofday(&end, 0);
	printf("sum=%d\n", s);
	printf("ptls time=%lf\n", timediff(&start, &end));  
}
