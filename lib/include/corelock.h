#ifndef CORELOCK_H
#define CORELOCK_H

#define _GNU_SOURCE
#include <sched.h>

#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Коды результатов CoreLock
 */
typedef enum {
  CL_OK = 0,
  CL_ERR_START = 128,
  CL_ERR_BUSY,
  CL_ERR_JOIN,
  CL_ERR_TERM,
} cl_status_t;

struct cl_instanse_s;

typedef long (*cl_task)(void *arg);

typedef enum {
  CL_OVERRUN_BH_STOP,
  CL_OVERRUN_BH_NOTIFY,
  CL_OVERRUN_BH_IGNORE,
} cl_overrun_bh;

typedef struct {
  size_t period_us;
  int priority;
  cl_overrun_bh or_bh;
  cpu_set_t *cpu_mask;
  size_t cpu_mask_size;
  int sched_policy;
} cl_attr_t;

#define cl_make_def_attrs(period, aff_mask, aff_mask_sz)                       \
  {.period_us = (period),                                                      \
   .priority = 80,                                                             \
   .cpu_mask = (aff_mask),                                                     \
   .cpu_mask_size = (aff_mask_sz),                                             \
   .or_bh = CL_OVERRUN_BH_STOP,\
   .sched_policy = SCHED_FIFO}

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
 * @return struct cl_instanse_s* Pointer to the initialized instance, or NULL on allocation failure.
 */
struct cl_instanse_s *cl_inst_create(cl_task task, void *arg, const cl_attr_t *attrs);

/**
 * @brief Spawns the real-time thread and starts periodic execution.
 * 
 * @param inst Pointer to the initialized CoreLock instance.
 * @return [[nodiscard]] CL_OK on success, CL_ERR_START if thread creation fails.
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
 * Standard blocking join operation. This must be called before cl_inst_destroy().
 * 
 * @param inst Pointer to the CoreLock instance.
 * @param ret [out] Pointer to store the long value returned by the task function.
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
 * @return CL_OK on success, CL_ERR_BUSY if the instance has not been joined yet.
 */
cl_status_t cl_inst_destroy(struct cl_instanse_s *inst);

#ifdef __cplusplus
}
#endif

#endif // CORELOCK_H
