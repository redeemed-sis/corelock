#ifndef CL_UTILS_H
#define CL_UTILS_H

#include "corelock.h"

int default_main_attr(cl_task step_fn, void* step_arg, cl_attr_t* attrs, int argc, char* argv[]);
int default_main(cl_task step_fn, void* step_arg, size_t period_us, int argc, char* argv[]);

#endif
