#include "corelock.h"
#include <sched.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

int default_main_attr(cl_task step_fn, void* step_arg, cl_attr_t* attrs, int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Put the cpu number to bind!\n");
    return -1;
  }
  char* endptr;
  int cpu = strtol(argv[1], &endptr, 10);
  cpu_set_t cpus;
  CPU_SET(cpu, &cpus);
  attrs->cpu_mask = &cpus;
  attrs->cpu_mask_size = sizeof(cpus);
  struct cl_instanse_s* ctx = cl_inst_create(step_fn, step_arg, attrs);
  if (cl_inst_run(ctx) != CL_OK)
    return -1;
  if (cl_inst_join(ctx, NULL) != CL_OK)
    return -1;
  cl_inst_destroy(ctx);
  return 0;
}

int default_main(cl_task step_fn, void* step_arg, size_t period_us, int argc, char* argv[]) {
  cl_attr_t attrs = cl_make_def_attrs(period_us, NULL, 0);
  attrs.or_bh = CL_OVERRUN_BH_NOTIFY;
  return default_main_attr(step_fn, step_arg, &attrs, argc, argv);
}