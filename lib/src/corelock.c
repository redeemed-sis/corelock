// Local headers
#include "corelock.h"

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CL_UNUSED(x) (void)(x)

typedef struct cl_instanse_s {
  cl_attr_t attrs;
  cl_task task;
  void *arg;
  atomic_int stop_flag;
  atomic_int is_joined;
  atomic_int is_finished;
  pthread_attr_t th_attr;
  pthread_t id;
  struct timespec start_time;
  void (*overrun_handler)(struct cl_instanse_s *, struct timespec *,
                          struct timespec *);
} cl_instanse;

static long long diff_nsecs(struct timespec *newer, struct timespec *older) {
  long long diff_sec = (long long)newer->tv_sec - older->tv_sec;
  long long diff_nsec = (long long)newer->tv_nsec - older->tv_nsec;

  if (diff_nsec < 0) {
    diff_sec--;
    diff_nsec += 1000000000LL;
  }

  return (diff_sec * 1000000000LL) + diff_nsec;
}

static double seconds_from_start(cl_instanse *inst, struct timespec *curr) {
  long long diff_sec = (long long)curr->tv_sec - inst->start_time.tv_sec;
  long long diff_nsec = (long long)curr->tv_nsec - inst->start_time.tv_nsec;

  if (diff_nsec < 0) {
    diff_sec--;
    diff_nsec += 1000000000LL;
  }

  long long total_us = (diff_sec * 1000000LL) + (diff_nsec / 1000LL);

  return (double)total_us / 1000000.0;
}

static void overrun_handler_notify(struct cl_instanse_s *inst,
                                   struct timespec *curr,
                                   struct timespec *next) {
  fprintf(stderr,
          "Overrun is occured on %.6lf seconds from start! (overhead is %lld "
          "nanoseconds)\n",
          seconds_from_start(inst, curr), diff_nsecs(curr, next));
}

static void overrun_handler_stop(struct cl_instanse_s *inst,
                                 struct timespec *curr, struct timespec *next) {
  overrun_handler_notify(inst, curr, next);
  fprintf(stderr, "Terminating...\n");
  atomic_store_explicit(&inst->stop_flag, 1, memory_order_relaxed);
}

static void overrun_handler_ignore(struct cl_instanse_s *inst,
                                   struct timespec *curr,
                                   struct timespec *next) {
  CL_UNUSED(inst);
  CL_UNUSED(curr);
  CL_UNUSED(next);
  return;
}

static void *thread_fn(void *inst_arg) {
  struct timespec next_tick, curr_time;
  cl_instanse *inst = (cl_instanse *)inst_arg;
  size_t periond_ns = inst->attrs.period_us * 1000;
  void *res = NULL;

  clock_gettime(CLOCK_MONOTONIC, &inst->start_time);
  next_tick = inst->start_time;
  while (!atomic_load_explicit(&inst->stop_flag, memory_order_acquire)) {
    next_tick.tv_nsec += periond_ns;
    next_tick.tv_sec += next_tick.tv_nsec / 1000000000L;
    next_tick.tv_nsec %= 1000000000L;
    res = (void *)inst->task(inst->arg);
    if (res) {
      goto fn_out;
    }
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    if (diff_nsecs(&curr_time, &next_tick) > 0) {
      inst->overrun_handler(inst, &curr_time, &next_tick);
      continue;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_tick, NULL);
  }

fn_out:
  atomic_store_explicit(&inst->is_finished, 1, memory_order_release);
  return res;
}

struct cl_instanse_s *cl_inst_create(cl_task task, void *arg,
                                     const cl_attr_t *attrs) {
  cl_instanse *inst = calloc(1, sizeof(*inst));
  pthread_attr_t *th_attr;
  struct sched_param param = {.sched_priority = attrs->priority};
  if (!inst)
    return NULL;

  memcpy(&inst->attrs, attrs, sizeof(inst->attrs));
  inst->task = task;
  inst->arg = arg;

  th_attr = &inst->th_attr;
  pthread_attr_init(th_attr);
  pthread_attr_setaffinity_np(th_attr, attrs->cpu_mask_size, attrs->cpu_mask);
  pthread_attr_setinheritsched(th_attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(th_attr, attrs->sched_policy);
  pthread_attr_setschedparam(th_attr, &param);

  switch (attrs->or_bh) {
    case CL_OVERRUN_BH_NOTIFY:
      inst->overrun_handler = overrun_handler_notify;
      break;
    case CL_OVERRUN_BH_IGNORE:
      inst->overrun_handler = overrun_handler_ignore;
      break;
    case CL_OVERRUN_BH_STOP:
      inst->overrun_handler = overrun_handler_stop;
      break;
  }

  return inst;
}

cl_status_t cl_inst_run(struct cl_instanse_s *inst) {
  int res = pthread_create(&inst->id, &inst->th_attr, thread_fn, inst);
  if (res)
    return CL_ERR_START;
  return CL_OK;
}

cl_status_t cl_inst_stop(struct cl_instanse_s *inst) {
  atomic_store_explicit(&inst->stop_flag, 1, memory_order_release);
  return CL_OK;
}

bool cl_inst_is_stopped(struct cl_instanse_s *inst) {
  return atomic_load_explicit(&inst->is_finished, memory_order_acquire);
}

cl_status_t cl_inst_join(struct cl_instanse_s *inst, long *ret) {
  void *th_ret;
  if (pthread_join(inst->id, &th_ret))
    return CL_ERR_JOIN;
  if (ret)
    *ret = (long)th_ret;
  atomic_store_explicit(&inst->is_joined, 1, memory_order_relaxed);
  return CL_OK;
}

cl_status_t cl_inst_term(struct cl_instanse_s *inst) {
  if (pthread_cancel(inst->id))
    return CL_ERR_TERM;
  return CL_OK;
}

cl_status_t cl_inst_destroy(struct cl_instanse_s *inst) {
  if (!atomic_load_explicit(&inst->is_joined, memory_order_relaxed)) {
    return CL_ERR_BUSY;
  }
  pthread_attr_destroy(&inst->th_attr);
  free(inst);
  return CL_OK;
}
