#include "corelock.h"
#include <sched.h>

static long step(void* arg) {
  (void) arg;
  return 0;
}

int main(int argc, char* argv[]) {
  (void) argc;
  (void) argv;

  cpu_set_t cpus;
  CPU_SET(15, &cpus);
  cl_attr_t attrs = cl_make_def_attrs(100, &cpus, sizeof(cpus));
  attrs.or_bh = CL_OVERRUN_BH_NOTIFY;
  struct cl_instanse_s* ctx = cl_inst_create(step, NULL, &attrs);
  if (cl_inst_run(ctx) != CL_OK)
    return -1;
  if (cl_inst_join(ctx, NULL) != CL_OK)
    return -1;
  cl_inst_destroy(ctx);
  return 0;
}
