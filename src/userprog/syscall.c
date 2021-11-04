#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define ARGV(sp, i) (*(uint32_t*)((sp)+4*(i)))

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  printf ("system call!\n");

  /* <-- Project 2 : System Call Start --> */
  void* sp = f->esp; // System call number
  // check_sp_validation(sp);

  int syscall_number = *(uint32_t *)sp;
  void* argv[3];

  // switch(syscall_number){
  //   case SYS_HALT:
  //     halt();
  //     break;
      
  //   case SYS_EXIT:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     exit(argv[0]); 
  //     break;
      
  //   case SYS_EXEC:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     exec(argv[0]); 
  //     break;
      
  //   case SYS_WAIT:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     wait(argv[0]); 
  //     break;
      
  //   case SYS_CREATE:
  //     cpy_arg(argv, sp, 2);
  //     check_sp_validation(argv, 2);
  //     create(argv[0], argv[1]); 
  //     break;
      
  //   case SYS_REMOVE:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     remove(argv[0]); 
  //     break;
      
  //   case SYS_OPEN:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     open(argv[0]); 
  //     break;
      
  //   case SYS_FILESIZE:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     filesize(argv[0]); 
  //     break;
      
  //   case SYS_READ:
  //     cpy_arg(argv, sp, 3);
  //     check_sp_validation(argv, 3);
  //     read(argv[0], argv[1], argv[2]); 
  //     break;
      
  //   case SYS_WRITE:
  //     cpy_arg(argv, sp, 3);
  //     check_sp_validation(argv, 3);
  //     write(argv[0], argv[1], argv[2]); 
  //     break;
      
  //   case SYS_SEEK:
  //     cpy_arg(argv, sp, 2);
  //     check_sp_validation(argv, 2);
  //     seek(argv[0], argv[1]); 
  //     break;
      
  //   case SYS_TELL:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     tell(argv[0]); 
  //     break;
      
  //   case SYS_CLOSE:
  //     cpy_arg(argv, sp, 1);
  //     check_sp_validation(argv, 1);
  //     close(argv[0]); 
  //     break;
      
  // }

  /* <-- Project 2 : System Call End --> */
}

void cpy_arg(void** argv, void **sp, int count)
{
  int i;
  for(i=0;i<count;i++)
  {
    *(argv+i)=*(sp+i);
  }
}

int check_sp_validation(void * sp, int count)
{
  int i;
  for(i=0;i<count;i++)
  {
    // if ( !is_user_vaddr(sp+4*i) ) exit(-1);
  }
}

void halt (void){
  shutdown_power_off();
};

void exit (int status) {
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

pid_t exec (const char *file){

}

int wait (pid_t pid){

}

bool create (const char *file, unsigned initial_size){

}

bool remove (const char *file){

}

int open (const char *file){

}

int filesize (int fd){

}

int read (int fd, void *buffer, unsigned length){

}

int write (int fd, const void *buffer, unsigned length){

}

void seek (int fd, unsigned position){

}

unsigned tell (int fd){

}

void close (int fd){

}
