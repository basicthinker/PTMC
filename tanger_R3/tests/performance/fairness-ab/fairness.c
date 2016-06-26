/*
 * File:
 *   fairness.c
 * Author(s):
 *   
 * Description:
 *   .
 *
 * Copyright (c) 2007-2009.
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

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#ifdef DEBUG
# define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define STORE(addr, value) (*(addr) = (value))
#define LOAD(addr) (*(addr))

#define DEFAULT_DURATION                10000
#define DEFAULT_NB_THREADS              1

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

static volatile int stop;

/* ################################################################### *
 * ELEMENTS
 * ################################################################### */

typedef struct element {
  uint64_t value;
  char padding[64-sizeof(uint64_t)];
} element_t;

typedef struct bench {
  element_t * elements;
  volatile int shared;
  char padding[64-sizeof(element_t *)];
} bench_t;

/* ################################################################### *
 * BARRIER
 * ################################################################### */

typedef struct barrier {
  pthread_cond_t complete;
  pthread_mutex_t mutex;
  int count;
  int crossing;
} barrier_t;

void barrier_init(barrier_t *b, int n)
{
  pthread_cond_init(&b->complete, NULL);
  pthread_mutex_init(&b->mutex, NULL);
  b->count = n;
  b->crossing = 0;
}

void barrier_cross(barrier_t *b)
{
  pthread_mutex_lock(&b->mutex);
  /* One more thread through */
  b->crossing++;
  /* If not all here, wait */
  if (b->crossing < b->count) {
    pthread_cond_wait(&b->complete, &b->mutex);
  } else {
    pthread_cond_broadcast(&b->complete);
    /* Reset for next time */
    b->crossing = 0;
  }
  pthread_mutex_unlock(&b->mutex);
}

/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data {
  int id;
  unsigned long nb_commits;
  unsigned int seed;
  int early;
  int not_shared;
  bench_t *bench;
  barrier_t *barrier;
  char padding[64];
} thread_data_t;

void *test(void *data)
{
  int i, j, nb;
  volatile int not_shared2;
  volatile int not_shared;
  volatile int *shared;
  thread_data_t *d = (thread_data_t *)data;

  if (d->not_shared) {
  	shared = &not_shared2;
  } else {
  	shared = &d->bench->shared;
  }
  
  /* Wait on barrier */
  barrier_cross(d->barrier);

  /* Number of read to do */
  nb = 8;
  for (i = 0; i < d->id ; i++) {
    nb *= 2;
  }

  if(d->early) {
    while (stop == 0) {
      j = d->id;
      __transaction {
        STORE(shared, j);
        /* Always read the same memory location to have the same validation cost (need NO_DUPLICATE_RW_SET to be defined) */
        for (i = 0;i < nb;i++)
          j += (int)LOAD(&not_shared);
      }
      d->nb_commits++;
    }
  } else {
    while (stop == 0) {
      j = d->id;
      __transaction {
        /* Always read the same memory location to have the same validation cost (need NO_DUPLICATE_RW_SET to be defined) */
        for (i = 0;i < nb;i++)
          j += (int)LOAD(&not_shared);
        STORE(shared, j);
      }
      d->nb_commits++;
    }
  }

  return NULL;
}

int main(int argc, char **argv)
{
  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"early",                     no_argument,       NULL, 'e'},
    {"local",                     no_argument,       NULL, 'l'},
    {"duration",                  required_argument, NULL, 'd'},
    {"num-threads",               required_argument, NULL, 'n'},
    {NULL, 0, NULL, 0}
  };

  bench_t *bench;
  int i, c;
  unsigned long commits;
  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  barrier_t barrier;
  struct timeval start, end;
  struct timespec timeout;
  int early = 0;
  int not_shared = 0;
  int duration = DEFAULT_DURATION;
  int nb_threads = DEFAULT_NB_THREADS;
  sigset_t block_set;

  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hled:n:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
     case 0:
       /* Flag is automatically set */
       break;
     case 'h':
       printf("fairness -- STM stress test\n"
              "\n"
              "Usage:\n"
              "  fairness [options...]\n"
              "\n"
              "Options:\n"
              "  -h, --help\n"
              "        Print this message\n"
              "  -d, --duration <int>\n"
              "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
              "  -l, --local\n"
              "        Local write, no conflict \n"
              "  -e, --early\n"
              "        Early write\n"
              "  -n, --num-threads <int>\n"
              "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
         );
       exit(0);
    case 'd':
       duration = atoi(optarg);
       break;
     case 'e':
       early = 1;
       break;
     case 'l':
       not_shared = 1;
       break;
     case 'n':
       nb_threads = atoi(optarg);
       break;
    case '?':
       printf("Use -h or --help for help\n");
       exit(0);
     default:
       exit(1);
    }
  }

  assert(duration >= 0);
  assert(nb_threads > 0);

  printf("Duration       : %d\n", duration);
  printf("Nb threads     : %d\n", nb_threads);
  printf("Type sizes     : int=%d/long=%d/ptr=%d/word=%d\n",
         (int)sizeof(int),
         (int)sizeof(long),
         (int)sizeof(void *),
         (int)sizeof(uintptr_t));

  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;

  if ((data = (thread_data_t *)malloc(nb_threads * sizeof(thread_data_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  if ((threads = (pthread_t *)malloc(nb_threads * sizeof(pthread_t))) == NULL) {
    perror("malloc");
    exit(1);
  }

  srand((int)time(0));

  bench = (bench_t *)malloc(sizeof(bench_t));
  bench->shared = 0;
//  bench->elements = (element_t *)malloc(100000 * sizeof(element_t));
//  bench->size = nb_elements;
//  for (i = 0; i < 10000; i++) {
//    bench->elements[i].value = i;
//  }

  stop = 0;

  /* Access set from all threads */
  barrier_init(&barrier, nb_threads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread %d\n", i);
    data[i].id = i;
    data[i].nb_commits = 0;
    data[i].seed = rand();
    data[i].early = early;
    data[i].not_shared = not_shared;
    data[i].bench = bench;
    data[i].barrier = &barrier;
    if (pthread_create(&threads[i], &attr, test, (void *)(&data[i])) != 0) {
      fprintf(stderr, "Error creating thread\n");
      exit(1);
    }
  }
  pthread_attr_destroy(&attr);
 
  /* Start threads */
  barrier_cross(&barrier);

  printf("STARTING...\n");
  gettimeofday(&start, NULL);
  if (duration > 0) {
    nanosleep(&timeout, NULL);
  } else {
    sigemptyset(&block_set);
    sigsuspend(&block_set);
  }
  stop = 1;
  gettimeofday(&end, NULL);
  printf("STOPPING...\n");

  /* Wait for thread completion */
  for (i = 0; i < nb_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
  }

  duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);

  for (i = 0; i < nb_threads; i++) {
    printf("Thread %d\n", i);
    printf("  #commits    : %lu\n", data[i].nb_commits);
    commits += data[i].nb_commits;
  }
  printf("Duration      : %d (ms)\n", duration);

  not_shared = 8;
  for (i = 0; i < nb_threads; i++) {
    printf("Thread %d (%d reads): %f percent (%lu commits)\n", i, not_shared, (double)data[i].nb_commits * 100.0 / (double)commits, data[i].nb_commits );
    not_shared *= 2;
  }

  /* Delete */
  //free(bench->accounts);
  free(bench);

  free(threads);
  free(data);

  return 0;
}
