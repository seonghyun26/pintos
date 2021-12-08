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
void check_valid_buffer_read(void* buffer, unsigned size);
void check_valid_buffer_write(void* buffer, unsigned size);
void check_valid_string(const void* str);
void get_argument(void *esp, uint32_t *arg , int count);
void check_file_name(const char *file);
void check_file_descriptor(struct file* fd);

/* <-- Project2 : System Call --> */


/* <-- Project3 : VM mmap Start --> */
mapid_t mmap (int fd, void* addr);
void munmap_file(struct mmap_file* mf);

struct mmap_file* mmap_file_create(void);
off_t mmap_write_back (struct file *f, void *kaddr, off_t ofs);
/* <-- Project3 : VM mmap End--> */

#endif /* userprog/syscall.h */
