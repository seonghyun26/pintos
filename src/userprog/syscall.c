#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_number;
  
  printf("syscall handler: %d\n", *(uint32_t *)(f->esp));
  thread_exit();

  switch (syscall_number)
  {
    case SYS_HALT:
      
      break;
    case SYS_EXIT:
      
      break;
    case SYS_EXEC:
      
      break;
    case SYS_WAIT:
      
      break;
    case SYS_CREATE:
      
      break;
    case SYS_REMOVE:
      
      break;
    case SYS_OPEN:
      
      break;
    case SYS_FILESIZE:
      
      break;
    case SYS_READ:
      
      break;
    case SYS_WRITE:
      
      break;
    case SYS_SEEK:
      
      break;
    case SYS_TELL:
      
      break;
    case SYS_CLOSE:
      
      break;
    default:
      exit(-1);
  }
}

// void check_valid_address(void* addr)
// {
//   if(!is_user_vaddr(addr))
//   {
//     exit(-1);
//   }
// }

// void get_argument(void *esp, int *arg , int count)
// {
//   int i;
//   for(i=0;i<count;i++)
//   {
//     check_valid_address(arg+i); /* 인자가 저장된 위치가 유저영역인지 확인 */
//     arg[i]=esp[i]; /* 유저 스택에 저장된 인자값들을 커널로 저장 */
//   }
// }

// void
// halt()
// {
//   shutdown_power_off();
// }

// void
// exit(int status)
// {
//   printf ("%s: exit(%d)\n",thread_name(),status);
// }

// pid_t
// exec (const char *cmd_line)
// {

// }

// int
// wait (pid_t pid)
// {

// }

// bool
// create (const char *file, unsigned initial_size)
// {
//   return filesys_create(file, initial_size);
// }

// bool remove (const char *file)
// {
//   return filesys_remove(file);
// }

// int open (const char *file)
// {

// }

// int filesize (int fd)
// {

// }

// int read (int fd, void *buffer, unsigned size)
// {

// }

// int write (int fd, const void *buffer, unsigned size)
// {

// }

// void seek (int fd, unsigned position)
// {

// }

// unsigned tell (int fd)
// {

// }

// void close (int fd)
// {

// }