#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include <stdint.h>
#include <string.h>
#include "vm/s_page.h"

void syscall_init (void);

/* <-- Project2 : System Call --> */

struct spte* check_valid_address(const void* addr);
void check_valid_buffer(void* buffer, unsigned size, bool write);
void check_valid_string(const void* str);
void get_argument(void *esp, uint32_t *arg , int count);
void check_file_name(const char *file);
void check_file_descriptor(struct file* fd);

int write (int fd, const void *buffer, unsigned size);
/* <-- Project2 : System Call --> */

#endif /* userprog/syscall.h */
