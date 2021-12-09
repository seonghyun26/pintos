#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <list.h>
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "process.h"
#include "vm/frame.h"
#include "vm/swap.h"

struct lock lock_filesys;
struct lock lock_mapid;
static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *vaddr);
static bool put_user (uint8_t *vaddr, uint8_t byte);

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
  thread_current()->sp = sp;

  int syscall_number = (int)*(uint32_t*)sp;
  uint32_t arg[3];
  
  // printf("\n\n>>> syscall handler: %d, esp: '%x'\n", syscall_number, (unsigned int)f->esp);
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
      // printf("\n--READ FLAG start--\n");
      check_valid_buffer_read((void *)arg[1],(unsigned)arg[2]);
      check_valid_buffer_write((void*)arg[1],(unsigned)arg[2]);
      // printf("\n--READ FLAG end--\n");
      f->eax = read(
        (int)arg[0],
        (void*)arg[1],
        (unsigned)arg[2]
      );
      break;
    case SYS_WRITE:   //9
      get_argument(sp , arg , 3);
      // printf("\n--WRITE FLAG start--\n");
      check_valid_buffer_read((void *)arg[1],(unsigned)arg[2]);
      // printf("\n--WRITE FLAG end--\n");
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
      // printf("<-- syscall mmap start -->\n");
      f->eax = mmap((int)arg[0], (void*)arg[1]);
      // printf("<-- syscall mmap end -->\n");
      break;
    case SYS_MUNMAP:  //14
      get_argument(sp , arg , 1);
      munmap((mapid_t)arg[0]);
      break;
    default:
      exit(-1);
  }

  // printf("Syscall Done!!\n\n");
}

struct spte* check_valid_address(const void* addr)
{
  // printf("Check Valid Address: %0x, %d\n", addr, is_user_vaddr(addr));
  if( !is_user_vaddr(addr) || (uint32_t*)addr < (uint32_t*)0x08048000) exit(-1);
  
  struct spte* check_spte = spte_find(thread_current(), addr);
  // printf("spte %x\n", check_spte);
  if( check_spte == NULL )  {
    // printf(">> No spte for addr %x\n", addr);
    exit(-1);
  }
  return check_spte;
}

void check_valid_buffer_read(void* buffer, unsigned size)
{
  void* tmp = pg_round_down(buffer);
  for(; tmp < buffer + size; tmp += PGSIZE)
  {
    if (get_user(tmp) == -1)
    {
      exit(-1);
    }
    struct spte* s = check_valid_address(tmp);
    if(s == NULL) exit(-1);
  }
}

void check_valid_buffer_write(void* buffer, unsigned size)
{
  void* tmp = pg_round_down(buffer);
  for(; tmp < buffer + size; tmp += PGSIZE)
  {
    // printf("tmp: %x\n", tmp);
    if (put_user(tmp, get_user(tmp)) == false)
    {
      exit(-1);
    }
    struct spte* s = check_valid_address(tmp);
    if(s == NULL) exit(-1);
    if(!s->writable) exit(-1);
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
  // printf(">>buffer %x\n", (void *)*((uint32_t*)(esp + 8)));

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
  // printf("in buffer: %x\n", *((uint32_t*)buffer));
  check_valid_address(buffer);
  if (fd == 0) 
  {
    i = input_getc();
  } 
  else if (fd > 2) 
  {
    lock_acquire(&lock_filesys);
    struct thread *cur = thread_current();
    check_file_descriptor(cur->fd[fd]);
    i = file_read(cur->fd[fd], buffer, size);
    lock_release(&lock_filesys);
  }
  return i;
}

int
write (int fd, const void *buffer, unsigned size)
{
  int v=-1;
  // printf(">>Write with FD: %d, at buffer %x, size: %d\n", fd, buffer, size);
  check_valid_address(buffer);
  
  if ( fd == 1 )
  {
    putbuf(buffer, size);
    v = size;
  }
  else if( fd > 2 )
  {
    lock_acquire(&lock_filesys);
    struct thread *cur = thread_current();
    check_file_descriptor(cur->fd[fd]);
    v = file_write(cur->fd[fd], buffer, size);
    lock_release(&lock_filesys);
  }
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
  // printf(">> A Flag...\n");

  if(addr==NULL || !is_user_vaddr(addr) || pg_ofs(addr) != 0)
    return -1;

  if( fd <= 1)  return -1;
  
  lock_acquire(&lock_filesys);
  // printf("fd: %d\n", fd);
  struct file *f = process_get_file(fd);
  if( f == NULL || !(f = file_reopen(f))) 
  {
    lock_release(&lock_filesys);
    file_close(f);
    return -1;
  }
  // printf(">> C Flag...\n");

  size_t size = file_length(f);
  if(size == 0)   
  {
    lock_release(&lock_filesys);
    file_close(f);
    return -1;
  }
  // printf(">> D Flag...\n");
  
  struct thread* cur = thread_current();
  struct spte* entry = spte_find(cur, addr);
  if(entry != NULL)
  {
    lock_release(&lock_filesys);
    file_close(f);
    return -1;
  }

  // Make a spte entry for mmap files
  off_t ofs;
  for(ofs = 0; (size_t) ofs < size; ofs += PGSIZE)
  {
    size_t read_bytes = ((size_t) ofs + PGSIZE < size) ? PGSIZE : size - ofs;
    size_t zero_bytes = PGSIZE - read_bytes;
    // printf("spte_file_create addr: %x", addr);
    struct spte* s= spte_file_create(addr + ofs, f, ofs, read_bytes, zero_bytes, true, true);
    if(s==NULL) break;
    //printf(" writable : %d\n",s->writable);
    hash_insert(cur->s_page_table, &s->elem);
  }
  // printf(">> start address: %x, end: %x, %x\n", addr, addr+ofs, size);

  if ( (size_t) ofs < size )
  {
    for(ofs = 0; (size_t) ofs < size; ofs += PGSIZE)
    {
      struct spte* spte_to_free = spte_find(cur,addr+ofs);
      hash_delete(cur->s_page_table, &spte_to_free->elem);
      free_spte(&spte_to_free->elem, 0);
      // printf("asdf\n");
    }
    lock_release(&lock_filesys);
    file_close(f);
    return -1;
  }

  // printf(">>MF create start\n");
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

  // printf(">>MMAP List insert\n");
  list_push_back(&cur->mmap_list,&new_mmap_file->elem);
  lock_release(&lock_filesys);
  
  return new_mmap_file->mapid;
}


void
munmap(mapid_t mapping)
{
  // printf(">>> UNMAP??\n");
  struct thread* cur = thread_current();
  struct mmap_file* mf = NULL;
  struct mmap_file* temp_mmap_file;
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
  if ( mf == NULL)  return;
  munmap_file(mf);
}

void
munmap_file(struct mmap_file* mf)
{
  // Unmap the mmap_file with mapid mapping
  off_t ofs;
  struct file* f = mf->file; 
  void* addr = mf->vaddress;
  size_t size = mf->size;

  lock_acquire(&lock_filesys);
  for(ofs = 0; (size_t) ofs < size; ofs += PGSIZE)
  {
    struct spte* spt_entry = spte_find(thread_current(),addr+ofs);
    if ( spt_entry == NULL )  continue;
    // printf("ofs: %d\n", ofs);
    update_spte_dirty(spt_entry);
    if ( spt_entry->kaddress != NULL)
    {
      //printf("Flag A\n");
      if(spt_entry->dirty)
      {
        file_write_at(f,spt_entry->kaddress, PGSIZE, ofs);
      }

      struct frame* frame_to_remove = frame_find_with_spte(spt_entry);
      if ( frame_to_remove != NULL){
        frame_free(frame_to_remove);
        pagedir_clear_page (spt_entry->pagedir, spt_entry->vaddress);
      }
      hash_delete(thread_current()->s_page_table, &spt_entry->elem);
      free_spte(&spt_entry->elem, 0);
      // printf("qwer\n");
    }
    else if ( spt_entry->type == PAGE_SWAP )
    {
      if(spt_entry->dirty)
      {
        void *kpage = palloc_get_page (0);
        swap_in (spt_entry->swap_idx, kpage);
        file_write_at (f, spt_entry->kaddress, PGSIZE, ofs);
        palloc_free_page (kpage);
      }
      // else
      // {
      //   swap_free(spt_entry->swap_idx);
      // }
      hash_delete(thread_current()->s_page_table, &spt_entry->elem);
      free_spte(&spt_entry->elem, 0);
      // printf("asdfqwer\n");
    }
  }

  list_remove(&mf->elem);
  free(mf);
  file_close(f);
  lock_release(&lock_filesys);
}

off_t
mmap_write_back (struct file *f, void *kaddr, off_t ofs)
{
  lock_acquire (&lock_filesys);
  f = file_reopen (f);
  if (f == NULL) return -1;
  off_t byte = file_write_at (f, kaddr, PGSIZE, ofs);
  lock_release (&lock_filesys);
  return byte;
}


/* <-- Project3 : VM mmap End--> */

static int
get_user (const uint8_t *vaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
  : "=&a" (result) : "m" (*vaddr));
  return result;
}

static bool
put_user (uint8_t *vaddr, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
  : "=&a" (error_code), "=m" (*vaddr) : "q" (byte));
  return error_code != -1;
}