#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "process.h"

struct lock lock_filesys;
struct lock lock_mapid;
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  lock_init(&lock_filesys);
  lock_init(&lock_mapid);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  void* sp = f -> esp; // user stack pointer
  // printf("sp: %x\n", sp);
  //printf("\n--- syscall FLAG---\n");

  check_valid_address(sp);
  
  int syscall_number = (int)*(uint32_t*)sp;
  uint32_t arg[3];
  
  // printf(">> syscall handler: %d, esp: '%x'\n", syscall_number, (unsigned int)f->esp);
  // hex_dump(f->esp, f->esp, 100, 1); 
  // thread_exit();
  
  switch (syscall_number)
  {
    case SYS_HALT:  //0
      halt();
      break;
    case SYS_EXIT:    //1
      get_argument(sp , arg , 1);
      exit((int)arg[0]);
      break;
    case SYS_EXEC:    //2
      get_argument(sp , arg , 1);
      // check_valid_string((const char *)arg[0]);
      f -> eax = exec((const char *)arg[0]);
      break;
    case SYS_WAIT:    //3
      get_argument(sp , arg , 1);
      f -> eax = wait((tid_t)arg[0]);
      break;
    case SYS_CREATE:  //4
      get_argument(sp , arg , 2);
      // check_valid_string((const char *)arg[0]);
      f->eax = create((const char *)arg[0], (unsigned)arg[1]);
      // printf("\n-- SYS CREATE COMPLETE -- \n");
      break;
    case SYS_REMOVE:  //5
      get_argument(sp , arg , 1);
      // check_valid_string((const char *)arg[0]);
      f->eax = remove((const char*)arg[0]);
      break;
    case SYS_OPEN:    //6
      get_argument(sp , arg , 1);
      // check_valid_string((const char *)arg[0]);
      f->eax = open((const char*)arg[0]);
      // printf("Open Returned %d\n", f->eax);
      break;
    case SYS_FILESIZE:  //7
      get_argument(sp , arg , 1);
      f->eax = filesize((int)arg[0]);
      break;
    case SYS_READ:    //8
      get_argument(sp , arg , 3);
      check_valid_buffer((void*)arg[1],(unsigned)arg[2],false);
      f->eax = read(
        (int)arg[0],
        (void*)arg[1],
        (unsigned)arg[2]
      );
      break;
    case SYS_WRITE:   //9
      get_argument(sp , arg , 3);
      check_valid_buffer((void *)arg[1],(unsigned)arg[2],true);
      f -> eax = write(
        (int)arg[0],
        (void *)arg[1],
        (unsigned)arg[2]
      );
      break;
    case SYS_SEEK:    //10
      get_argument(sp , arg , 2);
      seek((int)arg[0], (unsigned)arg[1]);
      break;
    case SYS_TELL:    //11
      get_argument(sp , arg , 1);
      f->eax = tell((int)arg[0]);
      break;
    case SYS_CLOSE:   //12
      get_argument(sp , arg , 1);
      close((int)arg[0]);
      break;
    case SYS_MMAP:    //13
      get_argument(sp , arg , 2);
      f->eax = mmap((int)arg[0], (void*)arg[1]);
      break;
    case SYS_MUNMAP:  //14
      get_argument(sp , arg , 1);
      munmap((mapid_t)arg[0]);
      break;
    default:
      exit(-1);
  }
}

struct spte* check_valid_address(const void* addr)
{
  // printf("Check Valid Address: %0x, %d\n", addr, is_user_vaddr(addr));
  if( !is_user_vaddr(addr) || (uint32_t*)addr < (uint32_t*)0x08048000) exit(-1);
  
  struct spte* check_spte = spte_find(thread_current(), addr);
  if( check_spte == NULL )  exit(-1);
  return check_spte;
}

void check_valid_buffer(void* buffer, unsigned size, bool write)
{
  void* tmp = buffer;
  // printf(">> size : %d\n",size);
  for(; tmp < buffer + size; tmp++)
  {
    // printf(">> tmp : %x\n", tmp);
    struct spte* s = check_valid_address(tmp);
    if(s == NULL) exit(-1);
    if(write && !s->writable) exit(-1);
  }
}

void check_valid_string(const void* str)
{
  const void* tmp = str;
  while(true)
  {
    struct spte* s = check_valid_address(tmp);
    if(s == NULL) exit(-1);
    if((*(char*)tmp) == '\0') break;
    tmp++;
  }
}

void get_argument(void *esp, uint32_t *arg , int count)
{
  int i;
  for( i = 1 ; i <= count ; i++ ) {
    check_valid_address(esp + i * 4);
    // Save arguments in stack to arg[]
    arg[i-1] = *((uint32_t*)(esp + 4*i));
  }
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

  lock_acquire(&lock_filesys);
  bool success = filesys_create(file, initial_size);
  lock_release(&lock_filesys);

  return success;
}

bool
remove (const char *file)
{
  check_file_name(file);

  lock_acquire(&lock_filesys);
  bool success = filesys_remove(file);
  lock_release(&lock_filesys);

  return success;
}

int
open (const char *file)
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

int
filesize (int fd)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  return file_length(cur->fd[fd]);
}

int
read (int fd, void *buffer, unsigned size)
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

int
write (int fd, const void *buffer, unsigned size)
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

void
seek (int fd, unsigned position)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  file_seek(cur->fd[fd], position);
}

unsigned
tell (int fd)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  return file_tell(cur->fd[fd]);
}

void
close (int fd)
{
  struct thread *cur = thread_current();
  check_file_descriptor(cur->fd[fd]);
  file_close(cur->fd[fd]);
  cur->fd[fd] = NULL;
}
/* <-- Project2 : System Call - File Manipulation End --> */ 


/* <-- Project3 : VM mmap Start--> */

static mapid_t
allocate_mapid (void) 
{
  static mapid_t next_mapid = 1;
  mapid_t mapid;

  lock_acquire (&lock_mapid);
  mapid = next_mapid++;
  lock_release (&lock_mapid);

  return mapid;
}

struct mmap_file*
mmap_file_create(void)
{
  struct mmap_file* new_mmap_file = malloc(sizeof(struct mmap_file));
  if(new_mmap_file == NULL) return NULL;
  new_mmap_file->mapid = allocate_mapid();
  return new_mmap_file;
}



mapid_t
mmap (int fd, void* addr)
{
  if(addr==NULL || !is_user_vaddr(addr) || pg_ofs(addr) != 0)
    return -1;
  
  lock_acquire(&lock_filesys);
  struct file *f = process_get_file(fd);
  if(f==NULL || !(f = file_reopen(f))) 
  {
    lock_release(&lock_filesys);
    return -1;
  }
  
  size_t size = file_length(f);
  if(size == 0)   
  {
    lock_release(&lock_filesys);
    file_close(f);
    return -1;
  }

  struct thread* cur = thread_current();
  // Make a spte entry for mmap files
  off_t ofs;
  for(ofs = 0; (size_t) ofs < size; ofs += PGSIZE)
  {
    size_t read_bytes = ((size_t) ofs + PGSIZE < size) ? PGSIZE : size - ofs;
    size_t zero_bytes = PGSIZE - read_bytes;
    struct spte* s= spte_file_create(addr + ofs, f, ofs, read_bytes, zero_bytes, true);
    if(s==NULL) break;
    hash_insert(cur->s_page_table, &s->elem);
  }
  if ( (size_t) ofs < size )
  {
    for(ofs = 0; (size_t) ofs < size; ofs += PGSIZE)
    {
      struct spte* spte_to_free = spte_find(cur,addr+ofs);
      hash_delete(cur->s_page_table, &spte_to_free->elem);
      free(spte_to_free);
    }
    lock_release(&lock_filesys);
    file_close(f);
    return -1;
  }
  
  struct mmap_file* new_mmap_file = mmap_file_create();
  if(new_mmap_file == NULL)
  {
    lock_release(&lock_filesys);
    file_close(f);
    return -1;
  }

  new_mmap_file->file=f;
  new_mmap_file->size=size;
  new_mmap_file->vaddress=addr;

  list_push_back(&cur->mmap_list,&new_mmap_file->elem);

  lock_release(&lock_filesys);
  return new_mmap_file->mapid;
}


void
munmap(mapid_t mapping)
{
  struct thread* cur = thread_current();
  struct mmap_file* mf;
  struct mmap_file* temp_mmap_file = NULL;
  struct list_elem* e;
  for ( e = list_begin(&cur->mmap_list) ; e != list_end(&cur->mmap_list) ; e = list_next(e))
  {
    temp_mmap_file = list_entry (e, struct mmap_file, elem);
    if ( temp_mmap_file->mapid == mapping)
    {
      mf = temp_mmap_file;
      break;
    }
  }
  // If there is no mmap_file with mapid mapping
  if ( temp_mmap_file == NULL)
  {
    exit(-1);
  }
  
  // Unmap the mmap_file with mapid mapping
  off_t ofs;
  struct file* f = mf->file; 
  void* addr = mf->vaddress;
  size_t size = mf->size;
  for(ofs = 0; (size_t) ofs < size; ofs += PGSIZE)
  {
    struct spte* s = spte_find(cur,addr+ofs);
    
    //if spte is dirty, write back.
    if(is_spte_dirty(s))
    {
      file_write_at(f,s->kaddress,PGSIZE, ofs);
    }
    pagedir_clear_page (s->pagedir, s->vaddress);
    hash_delete(cur->s_page_table, &s->elem);
    free(s);
  }
  list_remove(&mf->elem);
  free(mf);
  file_close(f);
  return;
}


/* <-- Project3 : VM mmap End--> */