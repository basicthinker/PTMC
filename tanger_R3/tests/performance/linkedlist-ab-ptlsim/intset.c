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
#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <unistd.h>

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#define PTLSIM_HYPERVISOR
#include "ptlcalls.h"

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

void pin_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    sched_setaffinity(0, sizeof(set), &set);
}

/* ################################################################### *
 * BARRIER
 * ################################################################### */

typedef struct {
    volatile unsigned long wait;
} barrier_t;

// TODO: We should be able to run this w/o the fences on x86, but maybe not on PTLsim!
static void inline atomic_inc(volatile unsigned long *a) {
    asm volatile ("lock incq %0"::"m"(*a):"memory");
}
static void inline atomic_dec(volatile unsigned long *a) {
    asm volatile ("lock decq %0"::"m"(*a):"memory");
}
static void barrier_init(barrier_t* b, unsigned long wait) {
    b->wait = wait;
    asm volatile ("mfence");
}
static void barrier_cross_nice(barrier_t* b) {
    atomic_dec(&b->wait);
    asm volatile ("mfence");
    while (b->wait) sched_yield();
    asm volatile ("mfence");
}
static void barrier_cross(barrier_t* b) {
    atomic_dec(&b->wait);
    asm volatile ("mfence");
    while (b->wait);
    asm volatile ("mfence");
}

/* ################################################################### *
 * INT SET
 * ################################################################### */

typedef struct node {
  intptr_t val;
  struct node *next;
} node_t;

typedef struct intset {
  node_t *head;
} intset_t;

node_t *new_node(intptr_t val, node_t *next)
{
  node_t *node;

  node = (node_t *)malloc(sizeof(node_t));
  if (node == NULL) {
    perror("malloc");
    exit(1);
  }

  node->val = val;
  node->next = next;

  return node;
}

intset_t *set_new()
{
  intset_t *set;
  node_t *min, *max;

  if ((set = (intset_t *)malloc(sizeof(intset_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  max = new_node(INT_MAX, NULL);
  min = new_node(INT_MIN, max);
  set->head = min;

  return set;
}

void set_delete(intset_t *set)
{
  node_t *node, *next;

  node = set->head;
  while (node != NULL) {
    next = node->next;
    free(node);
    node = next;
  }
  free(set);
}

int set_size(intset_t *set)
{
  int size = 0;
  node_t *node;

  /* We have at least 2 elements */
  node = set->head->next;
  while (node->next != NULL) {
    size++;
    node = node->next;
  }

  return size;
}

int set_add(intset_t *set, intptr_t val, int notx)
{
  int result;
  node_t *prev, *next;
  intptr_t v;
  node_t *n = NULL;

#ifdef DEBUG
  printf("++> set_add(%d)\n", val);
  IO_FLUSH;
#endif

  if (notx) {
    prev = set->head;
    next = prev->next;
    while (next->val < val) {
      prev = next;
      next = prev->next;
    }
    result = (next->val != val);
    if (result) {
      prev->next = new_node(val, next);
    }
  } else {
    n = new_node(val, next);
    __transaction {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = (intptr_t)LOAD(&next->val);
        if (v >= val)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
      }
      result = (v != val);
      if (result) {
        STORE(&n->val, val);
        STORE(&n->next, next);
        STORE(&prev->next, n);
      }
    }
    if (!result) free(n);
  }

  return result;
}

int set_remove(intset_t *set, intptr_t val, int notx)
{
  int result;
  node_t *prev, *next;
  intptr_t v;
  node_t *n;

#ifdef DEBUG
  printf("++> set_remove(%d)\n", val);
  IO_FLUSH;
#endif

  if (notx) {
    prev = set->head;
    next = prev->next;
    while (next->val < val) {
      prev = next;
      next = prev->next;
    }
    result = (next->val == val);
    if (result) {
      prev->next = next->next;
      free(next);
    }
  } else {
    __transaction {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = (intptr_t)LOAD(&next->val);
        if (v >= val)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
      }
      result = (v == val);
      if (result) {
        n = (node_t *)LOAD(&next->next);
        STORE(&prev->next, n);
        /* Overwrite deleted node */
        STORE(&next->val, 123456789);
        STORE(&next->next, next);
      }
    }
    /* Free the memory after committed execution */
    if (result)
      free(next);
  }

  return result;
}

int set_contains(intset_t *set, intptr_t val, int notx)
{
  int result;
  node_t *prev, *next;
  intptr_t v;

#ifdef DEBUG
  printf("++> set_contains(%d)\n", val);
  IO_FLUSH;
#endif

  if (notx) {
    prev = set->head;
    next = prev->next;
    while (next->val < val) {
      prev = next;
      next = prev->next;
    }
    result = (next->val == val);
  } else {
    __transaction {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = (intptr_t)LOAD(&next->val);
        if (v >= val)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
      }
      result = (v == val);
    }
  }

  return result;
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
  int total;
  int diff;
  int core;
  unsigned int seed;
  intset_t *set;
  barrier_t *init_barrier, *switch_barrier, *go_barrier;
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
  pin_cpu((int)d->core);

  barrier_cross_nice(d->init_barrier);
  barrier_cross_nice(d->switch_barrier);
  barrier_cross(d->go_barrier);

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
    if (d->nb_contains + d->nb_add + d->nb_remove >= d->total) stop = 1;
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
  cpu_set_t cpuset;
  barrier_t init_barrier, switch_barrier, go_barrier;
  const char* ptlcmd = "";

  struct timeval start, end;
  int duration = 10000;
  int initial = 256;
  int nb_threads = 1;
  int range = 0xFFFF;
  int seed = 0;
  int update = 20;

  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hd:i:n:r:s:u:p:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
     case 0:
       /* Flag is automatically set */
       break;
     case 'h':
       printf("Usage: %s [-d <int>] [-i <int>] [-n <int>] [-r <int>] [-s <int>] [-u <int>] [-p <string>]\n", argv[0]);
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
     case 'p':
       ptlcmd = optarg;
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
  barrier_init(&init_barrier,   nb_threads+1);
  barrier_init(&switch_barrier, nb_threads+1);
  barrier_init(&go_barrier,     nb_threads);

  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread %d\n", i);
    data[i].range = range;
    data[i].update = update;
    data[i].nb_add = 0;
    data[i].nb_remove = 0;
    data[i].nb_contains = 0;
    data[i].nb_found = 0;
    data[i].total = duration;
    data[i].diff = 0;
    data[i].core = i+1;
    data[i].seed = rand();
    data[i].set = set;
    data[i].init_barrier   = &init_barrier;
    data[i].switch_barrier = &switch_barrier;
    data[i].go_barrier     = &go_barrier;

    /* Pin threads early to CPUs to avoid additional balancing runs through Linux' scheduler */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    CPU_ZERO(&cpuset);
    CPU_SET(i+1, &cpuset);
    pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);

    if (pthread_create(&threads[i], &attr, test, (void *)(&data[i])) != 0) {
      fprintf(stderr, "Error creating thread\n");
      exit(1);
    }
  }
  pthread_attr_destroy(&attr);

  /* Start threads */
  barrier_cross(&init_barrier);
  printf("STARTING...\n");
  ptlcall_single_flush(ptlcmd);
  barrier_cross_nice(&switch_barrier);
  gettimeofday(&start, NULL);

  /* Wait for thread completion */
  for (i = 0; i < nb_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      exit(1);
    }
  }
  gettimeofday(&end, NULL);
  printf("STOPPING...\n");

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

  ptlcall_switch_to_native();

  /* Delete set */
  set_delete(set);

  free(threads);
  free(data);

  return 0;
}
