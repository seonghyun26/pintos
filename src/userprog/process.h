#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* <-- Project2 : Argument Passing Start --> */

void stack_argument(char **parse, int argc, void **rsp);    /* Saved arguments on Stack */
/* <-- Project2 : Argument Passing End --> */

#endif /* userprog/process.h */
