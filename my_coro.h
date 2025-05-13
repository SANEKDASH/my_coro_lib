#ifndef MY_CORO_LIB_HEADER
#define MY_CORO_LIB_HEADER

#define _GNU_SOURCE
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>

typedef enum {
  CORO_RUN,
  CORO_YIELD,
  CORO_FINISH,
  CORO_DESTROY,
} my_coro_state_t;

struct my_coro {
  ucontext_t routine_ctx;
  ucontext_t swap_ctx;

  size_t cpu_num;
  pthread_t thr;
  pthread_cond_t wait_run;
  pthread_mutex_t mt;
  
  my_coro_state_t state;

  void (*func)(void *);
  void *arg;
};

static const size_t DEFAULT_CORO_STACK_SIZE = 1024 * 16; // bytes 

int my_coro_init   (struct my_coro *c, size_t cpu_num, void (*func)(void *), void *arg);
int my_coro_run    (struct my_coro *c);
int my_coro_yield  (struct my_coro *c);
int my_coro_destroy(struct my_coro *c);
int my_coro_finish(struct my_coro *c);

#endif
