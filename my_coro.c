#include "my_coro.h"

static void *init_routine(void *coro_p);

int my_coro_init(struct my_coro *c, size_t cpu_num, void (*func)(void *), void *arg)
{
  int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
  if (cpu_num >= num_cpus) {
	return -1;
  }
  
  c->func = func;
  c->arg  = arg;  
  c->cpu_num = cpu_num;
  
  pthread_cond_init(&c->wait_run, NULL);
  pthread_mutex_init(&c->mt, NULL);
  
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
  c->routine_ctx.uc_link = &c->swap_ctx;
  makecontext(&c->routine_ctx, (void (*)(void)) c->func, 1, c->arg);

  c->state = CORO_YIELD;

  if (pthread_create(&c->thr, NULL, init_routine, c) < 0) {
	perror("failed to create thread\n");
	return -1;
  }

  return 0;
}

static void *init_routine(void *coro_p)
{
  struct my_coro *c = (struct my_coro *) coro_p;

  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  CPU_SET(c->cpu_num, &cpuset);

  if (pthread_setaffinity_np(c->thr, sizeof(cpu_set_t), &cpuset) != 0) {
	perror("pthread_setaffinity_np failed");
	return NULL;
  }

  while (1) {
	if (c->state == CORO_FINISH) {
	  return NULL;
	}
	
	pthread_mutex_lock(&c->mt);

	while (c->state != CORO_RUN) {
	  if (c->state == CORO_DESTROY) {
		return NULL;
	  }

	  pthread_cond_wait(&c->wait_run, &c->mt);
	}
	
	pthread_mutex_unlock(&c->mt);
	swapcontext(&c->swap_ctx, &c->routine_ctx);
  }
  
  return NULL;
}

int my_coro_run(struct my_coro *c)
{
  if (c->state == CORO_FINISH || c->state == CORO_DESTROY) {
	return -1;
  }
    
  pthread_mutex_lock(&c->mt);

  while (c->state != CORO_YIELD) {
	pthread_cond_wait(&c->wait_run, &c->mt);
  }  

  c->state = CORO_RUN;
  pthread_cond_signal(&c->wait_run);
  
  pthread_mutex_unlock(&c->mt);
  
  return 0;
}

int my_coro_run_with(struct my_coro *c, void *new_data)
{
  if (c->state == CORO_FINISH || c->state == CORO_DESTROY) {
	return -1;
  }
    
  pthread_mutex_lock(&c->mt);

  while (c->state != CORO_YIELD) {
	pthread_cond_wait(&c->wait_run, &c->mt);
  }  

  c->state = CORO_RUN;
  c->arg = new_data;
  pthread_cond_signal(&c->wait_run);
  
  pthread_mutex_unlock(&c->mt);

  return 0;
}

void *my_coro_get_data(struct my_coro *c) 
{
  return c->arg;
}

int my_coro_wait_data(struct my_coro *c, void *data)
{
  if (c->state == CORO_FINISH || c->state == CORO_DESTROY) {
	return -1;
  }
    
  pthread_mutex_lock(&c->mt);

  if (c->arg != data) {
	return 0;
  }
  
  while (c->state != CORO_YIELD) {
	pthread_cond_wait(&c->wait_run, &c->mt);
  }  
  
  pthread_mutex_unlock(&c->mt);

  return 0;
}


int my_coro_yield(struct my_coro *c)
{ 
  pthread_mutex_lock(&c->mt);
  
  if (c->state == CORO_RUN) {
	c->state = CORO_YIELD;
  }
  
  pthread_cond_signal(&c->wait_run);
  pthread_mutex_unlock(&c->mt);  
  
  swapcontext(&c->routine_ctx, &c->swap_ctx);

  return 0;
}

int my_coro_destroy(struct my_coro *c)
{
  pthread_mutex_lock(&c->mt);

  while (c->state != CORO_YIELD) {
	pthread_cond_wait(&c->wait_run, &c->mt);
  }  
  
  c->state = CORO_DESTROY;
  pthread_cond_signal(&c->wait_run);
  pthread_mutex_unlock(&c->mt);
  
  pthread_join(c->thr, NULL); 

  pthread_cond_destroy(&c->wait_run);
  pthread_mutex_destroy(&c->mt);
  
  free(c->routine_ctx.uc_stack.ss_sp);
  
  c->func = NULL;
  c->arg = NULL;  

  return 0;
}

int my_coro_finish(struct my_coro *c)
{ 
  pthread_mutex_lock(&c->mt);

  c->state = CORO_FINISH;
  
  pthread_cond_signal(&c->wait_run);
  pthread_mutex_unlock(&c->mt);

  return 0;
}
