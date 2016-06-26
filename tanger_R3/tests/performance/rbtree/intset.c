/* Copyright (C) 2007  Pascal Felber
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include <tanger-stm.h>

/* #define DEBUG */

#ifdef DEBUG
#define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define STORE(addr, value) (*(addr) = (value))
#define LOAD(addr) (*(addr))

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

static int verbose_flag = 0;
static volatile int stop;

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
 * RBTREE
 * ################################################################### */

#include "rbtree.h"

typedef rbtree_t intset_t;

intset_t *set_new()
{
  return rbtree_alloc();
}

void set_delete(intset_t *set)
{
  rbtree_free(set);
}

int set_size(intset_t *set)
{
  if (!rbtree_verify(set, 0)) {
    printf("Validation failed!\n");
    exit(1);
  }

  return rbtree_getsize(set);
}

int set_add(intset_t *set, intptr_t val, int notx)
{
  int res;

  if (notx) {
    res = !rbtree_insert(set, val, val);
  } else {
    tanger_begin();
    res = !rbtree_insert(set, val, val);
    tanger_commit();
  }

  return res;
}

int set_remove(intset_t *set, intptr_t val, int notx)
{
  int res;

  if (notx) {
    res = rbtree_delete(set, val);
  } else {
    tanger_begin();
    res = rbtree_delete(set, val);
    tanger_commit();
  }

  return res;
}

int set_contains(intset_t *set, intptr_t val, int notx)
{
  int res;

  if (notx) {
    res = rbtree_contains(set, val);
  } else {
    tanger_begin();
    res = rbtree_contains(set, val);
    tanger_commit();
  }

  return res;
}

/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data {
  int range;
  int update;
  int nb_add;
  int nb_remove;
  int nb_contains;
  int nb_found;
  int diff;
  unsigned int seed;
  intset_t *set;
  barrier_t *barrier;
  char padding[64];
} thread_data_t;

void *test(void *data)
{
  int val, last = 0;
  thread_data_t *d = (thread_data_t *)data;
#if 1
  unsigned int seed = d->seed;
#define SEED seed
#else
#define SEED d->seed
#endif

  /* Wait on barrier */
  barrier_cross(d->barrier);

  last = -1;
  while (stop == 0) {
    if ((rand_r(&SEED) % 100) < d->update) {
      if (last < 0) {
        /* Add random value */
        val = (rand_r(&SEED) % d->range) + 1;
        if (set_add(d->set, val, 0)) {
          d->diff++;
          last = val;
        }
        d->nb_add++;
      } else {
        /* Remove last value */
        if (set_remove(d->set, last, 0))
          d->diff--;
        d->nb_remove++;
        last = -1;
      }
    } else {
      /* Look for random value */
      val = (rand_r(&SEED) % d->range) + 1;
      if (set_contains(d->set, val, 0))
        d->nb_found++;
      d->nb_contains++;
    }
  }

  return NULL;
}

int main(int argc, char **argv)
{
  struct option long_options[] = {
    // These options set a flag
    {"silent",                    no_argument,       &verbose_flag, 0},
    {"verbose",                   no_argument,       &verbose_flag, 1},
    {"debug",                     no_argument,       &verbose_flag, 2},
    {"DEBUG",                     no_argument,       &verbose_flag, 3},
    // These options don't set a flag
    {"help",                      no_argument,       0, 'h'},
    {"duration",                  required_argument, 0, 'd'},
    {"initial-size",              required_argument, 0, 'i'},
    {"num-threads",               required_argument, 0, 'n'},
    {"range",                     required_argument, 0, 'r'},
    {"seed",                      required_argument, 0, 's'},
    {"update-rate",               required_argument, 0, 'u'},
    {0, 0, 0, 0}
  };

  intset_t *set;
  int i, c, val, size, reads, updates;
  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  barrier_t barrier;
  struct timeval start, end;
  struct timespec timeout;
  int duration = 10000;
  int initial = 256;
  int nb_threads = 1;
  int range = 0xFFFF;
  int seed = 0;
  int update = 20;

  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hd:i:n:r:s:u:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
     case 0:
       /* Flag is automatically set */
       break;
     case 'h':
       printf("Usage: %s [-d <int>] [-i <int>] [-n <int>] [-r <int>] [-s <int>] [-u <int>]\n", argv[0]);
       exit(0);
     case 'd':
       duration = atoi(optarg);
       break;
     case 'i':
       initial = atoi(optarg);
       break;
     case 'n':
       nb_threads = atoi(optarg);
       break;
     case 'r':
       range = atoi(optarg);
       break;
     case 's':
       seed = atoi(optarg);
       break;
     case 'u':
       update = atoi(optarg);
       break;
     case '?':
       printf("Use -h or --help for help\n");
       exit(0);
     default:
       exit(1);
    }
  }

  assert(duration >= 0);
  assert(initial >= 0);
  assert(nb_threads > 0);
  assert(range > 0);
  assert(update >= 0 && update <= 100);

  printf("Duration     : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Nb threads   : %d\n", nb_threads);
  printf("Value range  : %d\n", range);
  printf("Seed         : %d\n", seed);
  printf("Update rate  : %d\n", update);
  printf("int/long/ptr size: %u/%u/%u\n", (unsigned)sizeof(int), (unsigned)sizeof(long), (unsigned)sizeof(void*));

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

  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);
  set = set_new();

  stop = 0;

  /* Populate set */
  printf("Adding %d entries to set\n", initial);
  for (i = 0; i < initial; i++) {
    val = (rand() % range) + 1;
    set_add(set, val, 1);
  }
  size = set_size(set);
  printf("Set size  : %d\n", size);

  /* Access set from all threads */
  barrier_init(&barrier, nb_threads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread %d\n", i);
    data[i].range = range;
    data[i].update = update;
    data[i].nb_add = 0;
    data[i].nb_remove = 0;
    data[i].nb_contains = 0;
    data[i].nb_found = 0;
    data[i].diff = 0;
    data[i].seed = rand();
    data[i].set = set;
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
  if (nanosleep(&timeout, NULL) != 0) {
    perror("nanosleep");
    exit(1);
  }
  pthread_mutex_lock(&barrier.mutex);
  stop = 1;
  pthread_mutex_unlock(&barrier.mutex);
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
  reads = 0;
  updates = 0;
  for (i = 0; i < nb_threads; i++) {
    printf("Thread %d\n", i);
    printf("  #add      : %d\n", data[i].nb_add);
    printf("  #remove   : %d\n", data[i].nb_remove);
    printf("  #contains : %d\n", data[i].nb_contains);
    printf("  #found    : %d\n", data[i].nb_found);
    updates += (data[i].nb_add + data[i].nb_remove);
    reads += data[i].nb_contains;
    size += data[i].diff;
  }
  printf("Set size    : %d (expected: %d)\n", set_size(set), size);
  printf("Duration    : %d (ms)\n", duration);
  printf("#txs        : %d (%f / s)\n", reads + updates, (reads + updates) * 1000.0 / duration);
  printf("#read txs   : %d (%f / s)\n", reads, reads * 1000.0 / duration);
  printf("#update txs : %d (%f / s)\n", updates, updates * 1000.0 / duration);

  /* Delete set */
  set_delete(set);

  free(threads);
  free(data);

  return 0;
}
