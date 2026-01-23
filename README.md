# CoreLock

**CoreLock** is a lightweight, high-performance C library for Linux designed to manage and execute hard real-time periodic tasks. 

It is optimized for execution on isolated CPU cores and provides robust mechanisms to minimize jitter, handle CPU affinity, and manage execution overruns in deterministic environments.

## Key Features

*   **Core Isolation:** Native support for CPU affinity via thread attributes.
*   **Cycle time control** You may set cycle with With an accuracy of 1 us, determinism is pretty high (50 us cycles keeps on `PREEMPT_RT` kernels).
*   **RT Scheduling:** Supports `SCHED_FIFO` and `SCHED_RR` policies with configurable priorities.
*   **Overrun Management:** Three distinct policies for timing violations: `IGNORE`, `NOTIFY`, and `STOP`.
*   **Phase Alignment:** Ability to align task start times to system clock boundaries.
*   **Thread Safety:** Utilizes C11/C23 atomic operations for low-latency control and status monitoring.
*   **Zero-Overhead:** Designed to minimize system calls within the hot path of the task loop.

## Project Structure

```text
corelock/
├── CMakeLists.txt
├── examples            # Directory with examples of library usage
│   ├── basic_usage.c
│   ├── CMakeLists.txt
│   ├── dummy_rt.c
│   ├── utils.c
│   └── utils.h
├── lib
│   ├── CMakeLists.txt
│   ├── include
│   │   └── corelock.h  # Public API and Doxygen documentation
│   └── src
│       └── corelock.c  # Implementation (Thread loop, Atomic flags)
├── LICENSE
└── README.md
```

## Building and Installation
The library requires a C compiler supporting **C23** or **C11** and `pthreads`.
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

## Basic Usage
```C
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
```

## Quick start
Launch basic usage example
```bash
git clone https://github.com/redeemed-sis/corelock.git
cd corelock
cmake . -B build && cmake --build build/
sudo ./build/examples/basic_usage
```
