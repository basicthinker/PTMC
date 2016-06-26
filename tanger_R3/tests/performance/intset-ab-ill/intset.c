/*
 * File:
 *   intset.c
 * Author(s):
 *   Pascal Felber <pascal.felber@unine.ch>
 *   Jons-Tobias Wamhoff <jons@inf.tu-dresden.de>
 * Description:
 *   Integer set stress test.
 *
 * Copyright (c) 2007-2010.
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
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#ifdef ENABLE_REPORTING
#include "tanger-stm-stats.h"
#endif

#ifdef DEBUG
# define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#if !(defined(USE_LINKEDLIST) || defined(USE_RBTREE) || defined(USE_SKIPLIST))
# error "Must define USE_LINKEDLIST or USE_RBTREE or USE_SKIPLIST"
#endif /* !(defined(USE_LINKEDLIST) || defined(USE_RBTREE) || defined(USE_SKIPLIST)) */

#define STORE(addr, value) (*(addr) = (value))
#define LOAD(addr) (*(addr))
#define MALLOC(size)                    malloc(size)
#define FREE(addr, size)                free(addr)

#define DEFAULT_DURATION                10000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   0xFFFF
#define DEFAULT_SEED                    0
#define DEFAULT_UPDATE                  20
#define DEFAULT_NONTERM_OP              0
#define DEFAULT_NONTERM_NOOP            0
#define DEFAULT_CRASHES                 0
#define DEFAULT_TIMEOUT                 1000

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define NONTERM_OP                      10
#define NONTERM_NOOP                    11
#define CRASH                           12

int pthread_kill(pthread_t, int) __attribute__ ((transaction_pure));
void* pthread_getspecific(pthread_key_t) __attribute__ ((transaction_pure));
void perror(const char*) __attribute__ ((transaction_pure));
void exit(int) __attribute__ ((transaction_pure));
int rand_r(unsigned*) __attribute__ ((transaction_pure));

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

static volatile int stop;
static unsigned int global_seed;
static volatile uint64_t startTime;
static volatile uint64_t injectTimeout;

extern void tanger_stm_stats_enable();
extern void tanger_stm_stats_disable();
extern void tanger_stm_stats_external_event();

static int checkTime(void) __attribute__ ((transaction_pure,noinline));

static int checkTime(void) {
  struct timeval time;
  gettimeofday(&time, 0);
  uint64_t elapsedTime = time.tv_sec * 1000000 + time.tv_usec - startTime;
  if (elapsedTime > injectTimeout && stop == 0) {
    tanger_stm_stats_enable();
  }
  if (elapsedTime > injectTimeout * 2 && stop == 0) {
    tanger_stm_stats_external_event();
    return 1;
  }

  return 0;
}

/* ################################################################### *
 * THREAD-LOCAL
 * ################################################################### */

#ifdef TLS
static __thread unsigned int *rng_seed;
#else /* ! TLS */
static pthread_key_t rng_seed_key;
#endif /* ! TLS */

#if defined(USE_LINKEDLIST)

/* ################################################################### *
 * LINKEDLIST
 * ################################################################### */

# define TRANSACTIONAL                  1

# define INIT_SET_PARAMETERS            /* Nothing */

typedef intptr_t val_t;
# define VAL_MIN                        INT_MIN
# define VAL_MAX                        INT_MAX

typedef struct node {
  val_t val;
  struct node *next;
} node_t;

typedef struct intset {
  node_t *head;
} intset_t;

node_t *new_node(val_t val, node_t *next)
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
  max = new_node(VAL_MAX, NULL);
  min = new_node(VAL_MIN, max);
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

int set_contains(intset_t *set, val_t val)
{
  int result;
  node_t *prev, *next;
  val_t v;

# ifdef DEBUG
  printf("++> set_contains(%d)\n", val);
  IO_FLUSH;
# endif

  __transaction {
    prev = (node_t *)LOAD(&set->head);
    next = (node_t *)LOAD(&prev->next);
    while (1) {
      v = LOAD(&next->val);
      if (v >= val)
        break;
      prev = next;
      next = (node_t *)LOAD(&prev->next);
    }
    result = (v == val);
  }

  return result;
}

int set_add(intset_t *set, val_t val, int transactional)
{
  int result;
  node_t *prev, *next;
  val_t v;
  volatile int itsTime = 0;

  if (transactional >= 10)
    itsTime = checkTime();

# ifdef DEBUG
  printf("++> set_add(%d)\n", val);
  IO_FLUSH;
# endif

  __transaction {
    prev = (node_t *)LOAD(&set->head);
    next = (node_t *)LOAD(&prev->next);
    while (1) {
      v = LOAD(&next->val);
      if (v >= val)
        break;
      prev = next;
      next = (node_t *)LOAD(&prev->next);
    }
    result = (v != val);
    if (result) {
      STORE(&prev->next, new_node(val, next));
    }
    if (transactional == CRASH && itsTime) {
      pthread_kill(pthread_self(), SIGILL);
    }
    if (transactional == NONTERM_OP && itsTime) {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = LOAD(&next->val);
        if (v == 0x999999999)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
        if (next == NULL) {
          prev = (node_t *)LOAD(&set->head);
          next = (node_t *)LOAD(&prev->next);
        }
      }
      STORE(&prev->next, new_node(val, next));
    }
    if (transactional == NONTERM_NOOP && itsTime)
      while (1) {}
  }

  return result;
}

int set_remove(intset_t *set, val_t val, int transactional)
{
  int result;
  node_t *prev, *next;
  val_t v;
  node_t *n;
  volatile int itsTime = 0;

  if (transactional >= 10)
    itsTime = checkTime();

# ifdef DEBUG
  printf("++> set_remove(%d)\n", val);
  IO_FLUSH;
# endif

  __transaction {
    prev = (node_t *)LOAD(&set->head);
    next = (node_t *)LOAD(&prev->next);
    while (1) {
      v = LOAD(&next->val);
      if (v >= val)
        break;
      prev = next;
      next = (node_t *)LOAD(&prev->next);
    }
    result = (v == val);
    if (result) {
      n = (node_t *)LOAD(&next->next);
      STORE(&prev->next, n);
      /* Free memory (delayed until commit) */
      FREE(next, sizeof(node_t));
    }
    if (transactional == CRASH && itsTime) {
      pthread_kill(pthread_self(), SIGILL);
    }
    if (transactional == NONTERM_OP && itsTime) {
      prev = (node_t *)LOAD(&set->head);
      next = (node_t *)LOAD(&prev->next);
      while (1) {
        v = LOAD(&next->val);
        if (v == 0x999999999)
          break;
        prev = next;
        next = (node_t *)LOAD(&prev->next);
        if (next == NULL) {
          prev = (node_t *)LOAD(&set->head);
          next = (node_t *)LOAD(&prev->next);
        }
      }
      STORE(&prev->next, new_node(val, next));
    }
    if (transactional == NONTERM_NOOP && itsTime)
      while (1) {}
  }

  return result;
}

#elif defined(USE_RBTREE)

/* ################################################################### *
 * RBTREE
 * ################################################################### */

# define TRANSACTIONAL                  1

# define INIT_SET_PARAMETERS            /* Nothing */

# define TM_ARGDECL_ALONE               /* Nothing */
# define TM_ARGDECL                     /* Nothing */
# define TM_ARG                         /* Nothing */
# define TM_ARG_ALONE                   /* Nothing */
# define TM_CALLABLE                    /* Nothing */

# define TM_SHARED_READ(var)            LOAD(&(var))
# define TM_SHARED_READ_P(var)          LOAD(&(var))

# define TM_SHARED_WRITE(var, val)      STORE(&(var), val)
# define TM_SHARED_WRITE_P(var, val)    STORE(&(var), val)

# define TM_MALLOC(size)                MALLOC(size)
# define TM_FREE(ptr)                   FREE(ptr, sizeof(*ptr))

# include "rbtree.h"

# include "rbtree.c"

typedef rbtree_t intset_t;
typedef intptr_t val_t;

static long compare(const void *a, const void *b)
{
  return ((val_t)a - (val_t)b);
}

intset_t *set_new()
{
  return rbtree_alloc(&compare);
}

void set_delete(intset_t *set)
{
  rbtree_free(set);
}

int set_size(intset_t *set)
{
  int size;
  node_t *n;

  if (!rbtree_verify(set, 0)) {
    printf("Validation failed!\n");
    exit(1);
  }

  size = 0;
  for (n = firstEntry(set); n != NULL; n = successor(n))
    size++;

  return size;
}

int set_contains(intset_t *set, val_t val)
{
  int result;

  __transaction {
    result = TMrbtree_contains(set, (void *)val);
  }

  return result;
}

int set_add(intset_t *set, val_t val, int transactional)
{
  int result;
  volatile int itsTime = 0;

  if (transactional >= 10)
    itsTime = checkTime();

  __transaction {
    result = TMrbtree_insert(set, (void *)val, (void *)val);
    if (transactional == CRASH && itsTime) {
      pthread_kill(pthread_self(), SIGILL);
    }
    if (transactional == NONTERM_OP && itsTime)
      while (1) {
        TMrbtree_contains(set, (void *)val);
      }
    if (transactional == NONTERM_NOOP && itsTime)
      while (1) {}
  }

  return result;
}

int set_remove(intset_t *set, val_t val, int transactional)
{
  int result;
  volatile int itsTime = 0;

  if (transactional >= 10)
    itsTime = checkTime();

  __transaction {
    result = TMrbtree_delete(set, (void *)val);
    if (transactional == CRASH && itsTime) {
      pthread_kill(pthread_self(), SIGILL);
    }
    if (transactional == NONTERM_OP && itsTime)
      while (1) {
        TMrbtree_contains(set, (void *)val);
      }
    if (transactional == NONTERM_NOOP && itsTime)
      while (1) {}
  }

  return result;
}

#elif defined(USE_SKIPLIST)

/* ################################################################### *
 * SKIPLIST
 * ################################################################### */

# define TRANSACTIONAL                  1

# define MAX_LEVEL                      64

# define INIT_SET_PARAMETERS            32, 50

typedef intptr_t val_t;
typedef intptr_t level_t;
# define VAL_MIN                        INT_MIN
# define VAL_MAX                        INT_MAX

typedef struct node {
  val_t val;
  level_t level;
  struct node **forward;
} node_t;

typedef struct intset {
  node_t *head;
  node_t *tail;
  level_t level;
  int prob;
  int max_level;
} intset_t;

inline int rand_100()
{
# ifdef TLS
  return rand_r(rng_seed) % 100;
# else /* ! TLS */
  return rand_r((unsigned int *)pthread_getspecific(rng_seed_key)) % 100;
# endif /* ! TLS */
}

int random_level(intset_t *set)
{
  int l = 0;
  while (l < set->max_level && rand_100() < set->prob)
    l++;
  return l;
}

node_t *new_node(val_t val, level_t level)
{
  node_t *node;

  node = (node_t *)malloc(sizeof(node_t));
  if (node == NULL) {
    perror("malloc");
    exit(1);
  }
  node->forward = (node_t **)malloc((level + 1) * sizeof(node_t *));
  if (node->forward == NULL) {
    perror("malloc");
    exit(1);
  }

  node->val = val;
  node->level = level;

  return node;
}

intset_t *set_new(level_t max_level, int prob)
{
  intset_t *set;
  int i;

  assert(max_level <= MAX_LEVEL);
  assert(prob >= 0 && prob <= 100);

  if ((set = (intset_t *)malloc(sizeof(intset_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  set->max_level = max_level;
  set->prob = prob;
  set->level = 0;
  /* Set head and tail are immutable */
  set->tail = new_node(VAL_MAX, max_level);
  set->head = new_node(VAL_MIN, max_level);
  for (i = 0; i <= max_level; i++) {
    set->head->forward[i] = set->tail;
    set->tail->forward[i] = NULL;
  }

  return set;
}

void set_delete(intset_t *set)
{
  node_t *node, *next;

  node = set->head;
  while (node != NULL) {
    next = node->forward[0];
    free(node->forward);
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
  node = set->head->forward[0];
  while (node->forward[0] != NULL) {
    size++;
    node = node->forward[0];
  }

  return size;
}

int set_contains(intset_t *set, val_t val)
{
  int result, i;
  node_t *node, *next;
  val_t v;

  __transaction {
    v = VAL_MIN; /* Avoid compiler warning (should not be necessary) */
    node = set->head;
    for (i = LOAD(&set->level); i >= 0; i--) {
      next = (node_t *)LOAD(&node->forward[i]);
      while (1) {
        v = LOAD(&next->val);
        if (v >= val)
          break;
        node = next;
        next = (node_t *)LOAD(&node->forward[i]);
      }
    }
    node = (node_t *)LOAD(&node->forward[0]);
    result = (v == val);
  }

  return result;
}

int set_add(intset_t *set, val_t val, int transactional)
{
  int result, i;
  node_t *update[MAX_LEVEL + 1];
  node_t *node, *next;
  level_t level, l;
  val_t v;

  __transaction {
    v = VAL_MIN; /* Avoid compiler warning (should not be necessary) */
    node = set->head;
    level = LOAD(&set->level);
    for (i = level; i >= 0; i--) {
      next = (node_t *)LOAD(&node->forward[i]);
      while (1) {
        v = LOAD(&next->val);
        if (v >= val)
          break;
        node = next;
        next = (node_t *)LOAD(&node->forward[i]);
      }
      update[i] = node;
    }
    node = (node_t *)LOAD(&node->forward[0]);

    if (v == val) {
      result = 0;
    } else {
      l = random_level(set);
      if (l > level) {
        for (i = level + 1; i <= l; i++)
          update[i] = set->head;
        STORE(&set->level, l);
      }
      node = new_node(val, l);
      for (i = 0; i <= l; i++) {
        node->forward[i] = (node_t *)LOAD(&update[i]->forward[i]);
        STORE(&update[i]->forward[i], node);
      }
      result = 1;
    }
    if (transactional == CRASH) {
      pthread_kill(pthread_self(), SIGILL);
    }
    if (transactional == NONTERM_OP)
      while (1) {
        next = (node_t *)LOAD(&node->forward);
        if (v == 0x999999999)
          STORE(&node->forward[0], (node_t *)next);
      }
    if (transactional == NONTERM_NOOP)
      while (1) {}
  }

  return result;
}

int set_remove(intset_t *set, val_t val, int transactional)
{
  int result, i;
  node_t *update[MAX_LEVEL + 1];
  node_t *node, *next;
  level_t level;
  val_t v;

  __transaction {
    v = VAL_MIN; /* Avoid compiler warning (should not be necessary) */
    node = set->head;
    level = LOAD(&set->level);
    for (i = level; i >= 0; i--) {
      next = (node_t *)LOAD(&node->forward[i]);
      while (1) {
        v = LOAD(&next->val);
        if (v >= val)
          break;
        node = next;
        next = (node_t *)LOAD(&node->forward[i]);
      }
      update[i] = node;
    }
    node = (node_t *)LOAD(&node->forward[0]);

    if (v != val) {
      result = 0;
    } else {
      for (i = 0; i <= level; i++) {
        if ((node_t *)LOAD(&update[i]->forward[i]) == node)
          STORE(&update[i]->forward[i], (node_t *)LOAD(&node->forward[i]));
      }
      i = level;
      while (i > 0 && (node_t *)LOAD(&set->head->forward[i]) == set->tail)
        i--;
      if (i != level)
        STORE(&set->level, i);
      /* Free memory (delayed until commit) */
      FREE(node->forward, (node->level + 1) * sizeof(node_t *));
      FREE(node, sizeof(node_t));
      result = 1;
    }
    if (transactional == CRASH) {
      pthread_kill(pthread_self(), SIGILL);
    }
    if (transactional == NONTERM_OP)
      while (1) {
        next = (node_t *)LOAD(&node->forward);
        if (v == 0x999999999)
          STORE(&node->forward[0], (node_t *)next);
      }
    if (transactional == NONTERM_NOOP)
      while (1) {}
  }

  return result;
}

#endif /* defined(USE_SKIPLIST) */

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
  int range;
  int update;
  int nonterm_op;
  int nonterm_noop;
  int crash;
  int diff;
  unsigned int seed;
  intset_t *set;
  barrier_t *barrier;
  unsigned long nb_add;
  unsigned long nb_remove;
  unsigned long nb_contains;
  unsigned long nb_found;
} thread_data_t;

void *test(void *data)
{
  int val, last = 0;
  thread_data_t *d = (thread_data_t *)data;
  int transactional = TRANSACTIONAL;

#ifdef TLS
  rng_seed = &d->seed;
#else /* ! TLS */
  pthread_setspecific(rng_seed_key, &d->seed);
#endif /* ! TLS */

  /* Wait on barrier */
  barrier_cross(d->barrier);

  last = -1;
  while (stop == 0) {
    val = rand_r(&d->seed) % 100;
    if (d->nonterm_op)
      transactional = NONTERM_OP;
    else if (d->nonterm_noop)
      transactional = NONTERM_NOOP;
    else if (d->crash)
      transactional = CRASH;
    if (val < d->update) {
      if (last < 0) {
        /* Add random value */
        val = (rand_r(&d->seed) % d->range) + 1;
        if (set_add(d->set, val, transactional)) {
          d->diff++;
          last = val;
        }
        d->nb_add++;
      } else {
        /* Remove last value */
        if (set_remove(d->set, last, transactional))
          d->diff--;
        d->nb_remove++;
        last = -1;
      }
    } else {
      /* Look for random value */
      val = (rand_r(&d->seed) % d->range) + 1;
      if (set_contains(d->set, val))
        d->nb_found++;
      d->nb_contains++;
    }
  }

  return NULL;
}

int main(int argc, char **argv)
{
  struct option long_options[] = {
    // These options don't set a flag
    {"help",                      no_argument,       NULL, 'h'},
    {"duration",                  required_argument, NULL, 'd'},
    {"initial-size",              required_argument, NULL, 'i'},
    {"num-threads",               required_argument, NULL, 'n'},
    {"range",                     required_argument, NULL, 'r'},
    {"seed",                      required_argument, NULL, 's'},
    {"update-rate",               required_argument, NULL, 'u'},
    {"nonterms-op",               required_argument, NULL, 'z'},
    {"nonterms-noop",             required_argument, NULL, 'o'},
    {"crashes",                   required_argument, NULL, 'c'},
    {"timeout",                   required_argument, NULL, 't'},
    {NULL, 0, NULL, 0}
  };

  intset_t *set;
  int i, c, val, size;
  unsigned long reads, updates;
  thread_data_t *data;
  pthread_t *threads;
  pthread_attr_t attr;
  barrier_t barrier;
  struct timeval start, end;
  struct timespec timeout;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = DEFAULT_NB_THREADS;
  int range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  int update = DEFAULT_UPDATE;
  int nonterms_op = DEFAULT_NONTERM_OP;
  int nonterms_noop = DEFAULT_NONTERM_NOOP;
  int crashes = DEFAULT_CRASHES;
  injectTimeout = DEFAULT_TIMEOUT;

  while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hd:i:n:r:s:u:z:o:c:t:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
     case 0:
       /* Flag is automatically set */
       break;
     case 'h':
       printf("intset -- STM stress test "
#if defined(USE_LINKEDLIST)
              "(linked list)\n"
#elif defined(USE_RBTREE)
              "(red-black tree)\n"
#elif defined(USE_SKIPLIST)
              "(skip list)\n"
#endif /* defined(USE_SKIPLIST) */
              "\n"
              "Usage:\n"
              "  intset [options...]\n"
              "\n"
              "Options:\n"
              "  -h, --help\n"
              "        Print this message\n"
              "  -d, --duration <int>\n"
              "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
              "  -i, --initial-size <int>\n"
              "        Number of elements to insert before test (default=" XSTR(DEFAULT_INITIAL) ")\n"
              "  -n, --num-threads <int>\n"
              "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
              "  -r, --range <int>\n"
              "        Range of integer values inserted in set (default=" XSTR(DEFAULT_RANGE) ")\n"
              "  -s, --seed <int>\n"
              "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
              "  -u, --update-rate <int>\n"
              "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
              "  -z, --nonterm-op <int>\n"
              "        Number of nonterm transactions with stm operations (default=" XSTR(DEFAULT_NONTERM_OP) ")\n"
              "  -o, --nonterm-noop <int>\n"
              "        Number of nonterm transactions without stm operations (default=" XSTR(DEFAULT_NONTERM_NOOP) ")\n"
              "  -c, --crashes <int>\n"
              "        Number of crashing threads (default=" XSTR(DEFAULT_CRASHES) ")\n"
              "  -t, --timeout <int>\n"
              "        Time in us to wait before injecting faults (default=" XSTR(DEFAULT_TIMEOUT) ")\n"
         );
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
     case 'z':
       nonterms_op = atoi(optarg);
       break;
     case 'o':
       nonterms_noop = atoi(optarg);
       break;
     case 'c':
       crashes = atoi(optarg);
       break;
     case 't':
       injectTimeout = atoi(optarg);
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
  assert(range > 0 && range >= initial);
  assert(update >= 0 && update <= 100);
  assert(nonterms_op >= 0);
  assert(nonterms_noop >= 0);
  assert(crashes >= 0);
  assert(nonterms_op + nonterms_noop + crashes <= nb_threads);

#if defined(USE_LINKEDLIST)
  printf("Set type     : linked list\n");
#elif defined(USE_RBTREE)
  printf("Set type     : red-black tree\n");
#elif defined(USE_SKIPLIST)
  printf("Set type     : skip list\n");
#endif /* defined(USE_SKIPLIST) */
  printf("Duration     : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Nb threads   : %d\n", nb_threads);
  printf("Value range  : %d\n", range);
  printf("Seed         : %d\n", seed);
  printf("Update rate  : %d\n", update);
  printf("Zombies op   : %d\n", nonterms_op);
  printf("Zombies noop : %d\n", nonterms_noop);
  printf("Crashes      : %d\n", crashes);
  printf("Type sizes   : int=%d/long=%d/ptr=%d/word=%d\n",
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

  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);

  set = set_new(INIT_SET_PARAMETERS);

  stop = 0;

  global_seed = rand();
#ifdef TLS
  rng_seed = &global_seed;
#else /* ! TLS */
  if (pthread_key_create(&rng_seed_key, NULL) != 0) {
    fprintf(stderr, "Error creating thread local\n");
    exit(1);
  }
  pthread_setspecific(rng_seed_key, &global_seed);
#endif /* ! TLS */

  /* Populate set */
  printf("Adding %d entries to set\n", initial);
  i = 0;
  while (i < initial) {
    val = (rand() % range) + 1;
    if (set_add(set, val, 0))
      i++;
  }
  size = set_size(set);
  printf("Set size     : %d\n", size);

  /* Access set from all threads */
  barrier_init(&barrier, nb_threads + 1);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (i = 0; i < nb_threads; i++) {
    printf("Creating thread %d\n", i);
    data[i].range = range;
    data[i].update = update;
    data[i].nonterm_op = 0;
    data[i].nonterm_noop = 0;
    data[i].crash = 0;
    data[i].diff = 0;
    data[i].seed = rand();
    data[i].set = set;
    data[i].nb_add = 0;
    data[i].nb_remove = 0;
    data[i].nb_contains = 0;
    data[i].nb_found = 0;
    data[i].barrier = &barrier;
    if (pthread_create(&threads[i], &attr, test, (void *)(&data[i])) != 0) {
      fprintf(stderr, "Error creating thread\n");
      exit(1);
    }
  }
  for (i = 0; i < nonterms_op; i++) {
    data[i].nonterm_op = 1;
  }
  for (i = nonterms_op; i < nonterms_noop; i++) {
    data[i].nonterm_noop = 1;
  }
  for (i = nb_threads - 1; i > nb_threads - crashes - 1; i--) {
    data[i].crash = 1;
  }
  pthread_attr_destroy(&attr);

  printf("STARTING...\n"); fflush(NULL);
  /* Start threads */
  barrier_cross(&barrier);

  gettimeofday(&start, NULL);
  startTime = start.tv_sec * 1000000 + start.tv_usec;
  nanosleep(&timeout, NULL);
  stop = 1;
  gettimeofday(&end, NULL);
  printf("STOPPING...\n");

  /* Wait for thread completion */
  for (i = 0; i < nb_threads; i++) {
    if (!(data[i].nonterm_op || data[i].nonterm_noop || data[i].crash)) {
      if (pthread_join(threads[i], NULL) != 0) {
        fprintf(stderr, "Error waiting for thread completion\n");
        exit(1);
      }
    }
  }

  duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
  reads = 0;
  updates = 0;
  for (i = 0; i < nb_threads; i++) {
    printf("Thread %d\n", i);
    printf("  #add        : %lu\n", data[i].nb_add);
    printf("  #remove     : %lu\n", data[i].nb_remove);
    printf("  #contains   : %lu\n", data[i].nb_contains);
    printf("  #found      : %lu\n", data[i].nb_found);
    updates += (data[i].nb_add + data[i].nb_remove);
    reads += data[i].nb_contains;
  }

  printf("Set size      : %d (expected: %d)\n", set_size(set), size);
  printf("Duration      : %d (ms)\n", duration);
  printf("#txs          : %lu (%f / s)\n", reads + updates, (reads + updates) * 1000.0 / duration);
  printf("#read txs     : %lu (%f / s)\n", reads, reads * 1000.0 / duration);
  printf("#update txs   : %lu (%f / s)\n", updates, updates * 1000.0 / duration);

#ifdef ENABLE_REPORTING
  void* rh = tanger_stm_report_start("app");
#if defined(USE_LINKEDLIST)
  tanger_stm_report_append_string(rh, "name", "linkedlist");
#elif defined(USE_RBTREE)
  tanger_stm_report_append_string(rh, "name", "rbtree");
#elif defined(USE_SKIPLIST)
  tanger_stm_report_append_string(rh, "name", "skiplist");
#elif defined(USE_HASHSET)
    tanger_stm_report_append_string(rh, "name", "hashset");
#endif
  tanger_stm_report_append_int(rh, "duration_ms", duration);
  tanger_stm_report_append_int(rh, "threads", nb_threads);
  tanger_stm_report_append_int(rh, "nonterms_op", nonterms_op);
  tanger_stm_report_append_int(rh, "nonterms_noop", nonterms_noop);
  tanger_stm_report_append_int(rh, "crashes", crashes);
  tanger_stm_report_append_int(rh, "size", initial);
  tanger_stm_report_append_int(rh, "updaterate", update);
  tanger_stm_report_append_int(rh, "range", range);
  tanger_stm_report_finish(rh, "app");
  rh = tanger_stm_report_start("throughput");
  tanger_stm_report_append_int(rh, "commits", reads + updates);
  tanger_stm_report_append_double(rh, "commits_per_s", ((double)reads + updates) *1000.0 / duration);
  tanger_stm_report_finish(rh, "throughput");
#endif

  /* Delete set */
  set_delete(set);

#ifndef TLS
  pthread_key_delete(rng_seed_key);
#endif /* ! TLS */

  free(threads);
  free(data);

  return 0;
}
