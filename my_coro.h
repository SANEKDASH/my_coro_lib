#ifndef MY_CORO_LIB_HEADER
#define MY_CORO_LIB_HEADER

#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum {
  CORO_RUN,
  CORO_YIELDED,
  CORO_FINISH,
} my_coro_state_t;

struct my_coro {
  ucontext_t routine_ctx;
  ucontext_t finish_ctx;
  ucontext_t swap_ctx;
  
  my_coro_state_t state;

  void (*func)(void *);
  void *arg;
};

static const size_t DEFAULT_CORO_STACK_SIZE = 1024 * 16; // bytes 

int my_coro_init   (struct my_coro *c, void (*func)(void *), void *arg);
int my_coro_destroy(struct my_coro *c);

#endif
