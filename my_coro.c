#include "my_coro.h"

static void my_coro_finish_routine(struct my_coro *c);

int my_coro_init(struct my_coro *c, void (*func)(void *), void *arg)
{
  c->func = func;
  c->arg  = arg;

  /*
   * creating func routine context
   */
  
  if (getcontext(&c->routine_ctx) < 0) {
	perror("failed to get context");
	return -1;
  }

  c->routine_ctx.uc_stack.ss_sp = (char *) malloc(DEFAULT_CORO_STACK_SIZE);

  if (c->routine_ctx.uc_stack.ss_sp == NULL) {
	perror("failed to alloc memory");
	return -1;
  }
  
  c->routine_ctx.uc_stack.ss_size = DEFAULT_CORO_STACK_SIZE;
  c->routine_ctx.uc_link = &c->finish_ctx;
  makecontext(&c->routine_ctx, (void (*)(void)) c->func, 1, c->arg);

  /*
   * creating finish routine context
   */
  
  if (getcontext(&c->finish_ctx) < 0) {
	perror("failed to get context");
	return -1;
  }
  
  c->finish_ctx.uc_stack.ss_sp = (char *) malloc(DEFAULT_CORO_STACK_SIZE);

  if (c->finish_ctx.uc_stack.ss_sp == NULL) {
	perror("failed to alloc memory");
	return -1;
  }
  
  c->finish_ctx.uc_stack.ss_size = DEFAULT_CORO_STACK_SIZE;
  c->finish_ctx.uc_link = &c->swap_ctx;
  makecontext(&c->finish_ctx, (void (*)(void)) my_coro_finish_routine, 1, c);
  
  c->state = CORO_YIELDED;
  
  return 0;
}

int my_coro_run(struct my_coro *c)
{
  if (c->state == CORO_FINISH) {
	return -1;
  }
  
  c->state = CORO_RUN;

  swapcontext(&c->swap_ctx, &c->routine_ctx);
  
  return 0;
}

static void my_coro_finish_routine(struct my_coro *c)
{
  c->state = CORO_FINISH;
}

int my_coro_yield(struct my_coro *c)
{
  c->state = CORO_YIELDED;
  
  swapcontext(&c->routine_ctx, &c->swap_ctx);

  return 0;
}

int my_coro_destroy(struct my_coro *c)
{
  free(c->routine_ctx.uc_stack.ss_sp);
  free(c->finish_ctx.uc_stack.ss_sp);
  
  c->func = NULL;
  c->arg = NULL;  

  return 0;
}
