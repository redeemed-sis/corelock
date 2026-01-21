#ifndef CORELOCK_H
#define CORELOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#define _GNU_SOURCE
#include <sched.h>

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief CoreLock Status and Error Codes.
 * 
 * Defines the return values for all CoreLock API functions. 
 * Values >= 128 indicate a failure state.
 */
typedef enum {
    /** @brief Operation completed successfully. */
    CL_OK = 0,
    
    /** @brief Failed to spawn the real-time thread or initialize scheduling. */
    CL_ERR_START = 128,
    
    /** @brief Resource is currently in use (e.g., trying to destroy a running task). */
    CL_ERR_BUSY,
    
    /** @brief Failed to join the thread (e.g., thread was already joined or invalid). */
    CL_ERR_JOIN,
    
    /** @brief Failed to terminate the thread forcefully. */
    CL_ERR_TERM,
} cl_status_t;

/**
 * @brief Opaque handle to a CoreLock task instance.
 * 
 * This structure encapsulates the internal state of a periodic task, including 
 * thread identifiers, atomic flags, and timing metadata. 
 * 
 * The internal fields are hidden from the user to ensure API stability and 
 * thread-safety. Use cl_inst_create() to allocate and initialize.
 */
struct cl_instanse_s;

/**
 * @brief Task callback function signature.
 * 
 * This function defines the logic to be executed periodically by the CoreLock thread.
 * 
 * @param arg Pointer to user-defined data passed during instance creation.
 * @return long User-defined status code. A non-zero return value will trigger 
 *              thread termination (exit from the periodic loop).
 */
typedef long (*cl_task)(void *arg);

/**
 * @brief Overrun Behavior (BH) policies.
 * 
 * Defines how the library reacts when a task execution exceeds its allocated 
 * time period (jitter or execution time > period_us).
 */
typedef enum {
    /** @brief Stop the task immediately and set the finished flag. */
    CL_OVERRUN_BH_STOP,
    /** @brief Print a notification to stderr and continue execution. */
    CL_OVERRUN_BH_NOTIFY,
    /** @brief Silently continue without any action. */
    CL_OVERRUN_BH_IGNORE,
} cl_overrun_bh;

/**
 * @brief Configuration attributes for a CoreLock instance.
 * 
 * This structure defines the real-time characteristics, CPU affinity, 
 * and execution constraints of the periodic task.
 */
typedef struct {
    /** @brief Execution period in microseconds (us). */
    size_t period_us;
    
    /** @brief Real-time priority (typically 1-99 for SCHED_FIFO/RR). */
    int priority;
    
    /** @brief Policy to apply when an overrun occurs. */
    cl_overrun_bh or_bh;
    
    /** @brief Pointer to the CPU affinity mask (cpu_set_t). */
    cpu_set_t *cpu_mask;
    
    /** @brief Size of the cpu_mask structure in bytes (use sizeof(cpu_set_t)). */
    size_t cpu_mask_size;
    
    /** @brief Scheduling policy (e.g., SCHED_FIFO, SCHED_RR, SCHED_OTHER). */
    int sched_policy;
    
    /** @brief Total duration in seconds after which the task stops. Use -1 for infinite. */
    double stop_time;
    
    /** @brief Start time alignment in nanoseconds (e.g., 1e9 for 1s boundary). Use 0 for immediate start. */
    int start_align;
} cl_attr_t;

/**
 * @brief Macro helper to initialize default attributes for a CoreLock task.
 * 
 * Sets the default priority to 80, policy to SCHED_FIFO, and overrun behavior to STOP.
 * 
 * @param period Task period in microseconds.
 * @param aff_mask Pointer to a pre-configured cpu_set_t mask.
 * @param aff_mask_sz Size of the cpu_set_t structure.
 */
#define cl_make_def_attrs(period, aff_mask, aff_mask_sz)                       \
  {.period_us = (period),                                                      \
   .priority = 80,                                                             \
   .cpu_mask = (aff_mask),                                                     \
   .cpu_mask_size = (aff_mask_sz),                                             \
   .or_bh = CL_OVERRUN_BH_STOP,                                                \
   .sched_policy = SCHED_FIFO,                                                 \
   .stop_time = -1,                                                            \
   .start_align = 0}

/**
 * @brief Creates and initializes a new CoreLock task instance.
 *
 * Allocates memory for the instance, copies attributes, and prepares
 * thread attributes (affinity, scheduler, priority). The task remains
 * in a dormant state until cl_inst_run() is called.
 *
 * @param task Pointer to the function to be executed periodically.
 * @param arg User-defined argument passed to the task.
 * @param attrs Configuration structure (period, priority, affinity, etc.).
 * @return struct cl_instanse_s* Pointer to the initialized instance, or NULL on
 * allocation failure.
 */
struct cl_instanse_s *cl_inst_create(cl_task task, void *arg,
                                     const cl_attr_t *attrs);

/**
 * @brief Spawns the real-time thread and starts periodic execution.
 *
 * @param inst Pointer to the initialized CoreLock instance.
 * @return [[nodiscard]] CL_OK on success, CL_ERR_START if thread creation
 * fails.
 */
cl_status_t cl_inst_run(struct cl_instanse_s *inst);

/**
 * @brief Signals a running task to stop gracefully.
 *
 * Sets the internal stop flag using atomic release semantics. The task will
 * finish its current iteration before exiting the loop.
 *
 * @param inst Pointer to the CoreLock instance.
 * @return CL_OK on success.
 */
cl_status_t cl_inst_stop(struct cl_instanse_s *inst);

/**
 * @brief Checks if the task thread has finished execution.
 *
 * Uses atomic acquire semantics to verify if the thread loop has terminated.
 *
 * @param inst Pointer to the CoreLock instance.
 * @return true if the task has finished, false otherwise.
 */
bool cl_inst_is_stopped(struct cl_instanse_s *inst);

/**
 * @brief Waits for the task thread to terminate and retrieves its return value.
 *
 * Standard blocking join operation. This must be called before
 * cl_inst_destroy().
 *
 * @param inst Pointer to the CoreLock instance.
 * @param ret [out] Pointer to store the long value returned by the task
 * function.
 * @return CL_OK on success, CL_ERR_JOIN if pthread_join fails.
 */
cl_status_t cl_inst_join(struct cl_instanse_s *inst, long *ret);

/**
 * @brief Forcefully terminates the task thread.
 *
 * Uses pthread_cancel() to kill the thread. Warning: this may leave resources
 * in an inconsistent state if the task does not have cancellation points.
 *
 * @param inst Pointer to the CoreLock instance.
 * @return CL_OK on success, CL_ERR_TERM on failure.
 */
cl_status_t cl_inst_term(struct cl_instanse_s *inst);

/**
 * @brief Deallocates all resources associated with the instance.
 *
 * Frees the instance memory and destroys thread attributes.
 * The task must be joined before calling this function.
 *
 * @param inst Pointer to the CoreLock instance.
 * @return CL_OK on success, CL_ERR_BUSY if the instance has not been joined
 * yet.
 */
cl_status_t cl_inst_destroy(struct cl_instanse_s *inst);

#ifdef __cplusplus
}
#endif

#endif // CORELOCK_H
