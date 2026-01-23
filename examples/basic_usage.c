#include <corelock.h>
#include <sched.h>
#include <unistd.h>

// User-defined real-time task
long my_rt_task(void *arg) {
    (void) arg;
    // Perform time-critical logic here
    return 0; // Return 0 to continue, non-zero to exit loop
}

int main() {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset); // Target an isolated core

    // Initialize attributes with 1ms period (1000us)
    cl_attr_t attrs = cl_make_def_attrs(1000, &cpuset, sizeof(cpuset));
    attrs.priority = 80;
    attrs.or_bh = CL_OVERRUN_BH_NOTIFY;

    struct cl_instanse_s *inst = cl_inst_create(my_rt_task, NULL, &attrs);
    
    // Start execution
    cl_inst_run(inst);
    
    // Main loop or wait
    sleep(10);
    
    // Graceful shutdown
    cl_inst_stop(inst);
    cl_inst_join(inst, NULL);
    cl_inst_destroy(inst);

    return 0;
}
