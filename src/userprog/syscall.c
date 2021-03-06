#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "process.h"

struct lock lock_filesys;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&lock_filesys);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  void* sp = f -> esp; // user stack pointer
  // printf("sp: %x\n", sp);
  check_valid_address(sp);
  int syscall_number = (int)*(uint32_t*)sp;
  uint32_t arg[3];
  
  // printf("syscall handler: %d, esp: '%x'\n", syscall_number, f->esp);
  // hex_dump(f->esp, f->esp, 100, 1); 
  // thread_exit();

  switch (syscall_number)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      get_argument(sp , arg , 1);
      exit((int)arg[0]);
      break;
    case SYS_EXEC:
      get_argument(sp , arg , 1);
      f -> eax = exec((const char *)arg[0]);
      break;
    case SYS_WAIT:
      get_argument(sp , arg , 1);
      f -> eax = wait((tid_t)arg[0]);
      break;
    case SYS_CREATE:
      get_argument(sp , arg , 2);
      f->eax = create((const char *)arg[0], (unsigned)arg[1]);
      break;
    case SYS_REMOVE:
      get_argument(sp , arg , 1);
      f->eax = remove((const char*)arg[0]);
      break;
    case SYS_OPEN:
      get_argument(sp , arg , 1);
      f->eax = open((const char*)arg[0]);
      // printf("Open Returned %d\n", f->eax);
      break;
    case SYS_FILESIZE:
      get_argument(sp , arg , 1);
      f->eax = filesize((int)arg[0]);
      break;
    case SYS_READ:
      get_argument(sp , arg , 3);
      f->eax = read(
        (int)arg[0],
        (void*)arg[1],
        (unsigned)arg[2]
      );
      break;
    case SYS_WRITE:
      get_argument(sp , arg , 3);
      f -> eax = write(
        (int)arg[0],
        (void *)arg[1],
        (unsigned)arg[2]
      );
      break;
    case SYS_SEEK:
      get_argument(sp , arg , 2);
      seek((int)arg[0], (unsigned)arg[1]);
      break;
    case SYS_TELL:
      get_argument(sp , arg , 1);
      f->eax = tell((int)arg[0]);
      break;
    case SYS_CLOSE:
      get_argument(sp , arg , 1);
      close((int)arg[0]);
      break;
    default:
      exit(-1);
  }
}

void check_valid_address(const void* addr)
{
  // printf("Check Valid Address: %d\n", is_user_vaddr(addr));
  if(!is_user_vaddr(addr)  ) exit(-1);
}

void get_argument(void *esp, uint32_t *arg , int count)
{
  int i;
  for( i = 1 ; i <= count ; i++ ) {
    // printf("%d at %d\n", *((uint32_t*)(esp + 4*i)), i);
    check_valid_address(esp + i * 4); /* ????????? ????????? ????????? ?????????????????? ?????? */
    // printf("ASDF %d\n", i);
    arg[i-1] = *((uint32_t*)(esp + 4*i)); /* ?????? ????????? ????????? ??????????????? ????????? ?????? */
    // printf("QWER %d\n", i);
  }

  // for ( i = 0 ; i < count ; i++ ){ 
  //   printf("argv %d : %x\n", i, arg[i]);
  // }
}

/* <-- Project2 : System Call - User Process Manipulation Start --> */
void
halt()
{
  shutdown_power_off();
}

void
exit(int status)
{
  struct thread *cur = thread_current();
  cur->exit_status = status;
  printf ("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

tid_t
exec (const char *cmd_line)
{
  tid_t pid = process_execute(cmd_line);
  if( pid == TID_ERROR ) return pid;

  struct thread *child = get_child_process(pid);
  sema_down(&child->sema_load);
  if(!child->program_loaded) return TID_ERROR;

  return pid;
}

int
wait (tid_t pid)
{
  if ( thread_current()->tid == pid ) return -1;
  // printf(">> WAIT pid: %d\n", pid);
  return process_wait(pid);
}
/* <-- Project2 : System Call - User Process Manipulation End --> */


/* <-- Project2 : System Call - File Manipulation Start --> */
void check_file_name(const char *file)
{
  if (file == NULL) 
  {
    exit(-1);
  }
}

void check_file_descriptor(struct file* fd)
{
  if (fd == NULL) 
  {
    exit(-1);
  }
}

bool
create (const char *file, unsigned initial_size)
{
  check_file_name(file);
  return filesys_create(file, initial_size);
}

bool remove (const char *file)
{
  check_file_name(file);
  return filesys_remove(file);
}

int open (const char *file)
{
  check_file_name(file);
  lock_acquire(&lock_filesys);
  int v;
  struct file* fp = filesys_open(file);
  if (fp == NULL) 
    v = -1; 
  else 
  {
    struct thread *cur = thread_current();
    if (strcmp(cur->name, file) == 0)
      file_deny_write(fp);
    cur->fd[++cur->fd_count]=fp;
    v = cur->fd_count;
  }
  lock_release(&lock_filesys);
  return v; 
}

int filesize (int fd)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  return file_length(cur->fd[fd]);
}

int read (int fd, void *buffer, unsigned size)
{
  int i = -1;
  check_valid_address(buffer);
  lock_acquire(&lock_filesys);
  if (fd == 0) 
  {
    // for (i = 0; i < (long long int)size; i ++) 
    //   if (((char *)buffer)[i] == '\0') break;
    i = input_getc();
  } 
  else if (fd > 2) 
  {
    struct thread *cur = thread_current();
    check_file_descriptor(cur->fd[fd]);
    i = file_read(cur->fd[fd], buffer, size);
  }
  lock_release(&lock_filesys);
  return i;
}

int write (int fd, const void *buffer, unsigned size)
{
  int v=-1;
  check_valid_address(buffer);
  lock_acquire(&lock_filesys);
  if ( fd == 1 )
  {
    putbuf(buffer, size);
    v = size;
  }
  else if( fd > 2 )
  {
    struct thread *cur = thread_current();
    check_file_descriptor(cur->fd[fd]);
    // if(!cur->fd[fd]->deny_write) 
    // {
    // }
    v = file_write(cur->fd[fd], buffer, size);
  }
  lock_release(&lock_filesys);
  return v;
}

void seek (int fd, unsigned position)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  file_seek(cur->fd[fd], position);
}

unsigned tell (int fd)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  return file_tell(cur->fd[fd]);
}

void close (int fd)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  file_close(cur->fd[fd]);
  cur->fd[fd] = NULL;
}
/* <-- Project2 : System Call - File Manipulation End --> */ 