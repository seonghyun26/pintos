#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/* Process identifier. */
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

/* <-- Project 2 : System Call Start --> */
void cpy_arg(void** argv, void **sp, int count);
int check_sp_validation(void * sp, int count);
/* <-- Project 2 : System Call End --> */

#endif /* userprog/syscall.h */
