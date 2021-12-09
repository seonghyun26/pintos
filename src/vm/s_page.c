#include "vm/s_page.h"
#include <hash.h>
#include <stdio.h>
#include <stdlib.h>
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "threads/pte.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/swap.h"


static bool install_page (void *upage, void *kpage, bool writable);

struct hash*
s_page_table_init(void)
{
  struct hash* new_s_page_table = malloc(sizeof(struct hash));
  hash_init(new_s_page_table, s_page_table_hash, s_page_table_less_func, 0);

  return new_s_page_table;
}


struct spte* 
spte_file_create(
  uint32_t* vaddress, struct file* file, off_t ofs,
  size_t read_bytes, size_t zero_bytes, bool writable, bool mmap
)
{
  if(spte_find(thread_current(),vaddress) != NULL) return NULL;
  struct spte* new_spte = malloc(sizeof(struct spte));
  if ( new_spte == NULL ) return NULL;
  
  new_spte->vaddress=pg_round_down(vaddress);
  // printf(">> New spt_entry->vaddress: %x\n", new_spte->vaddress);
  new_spte->kaddress=NULL;
  new_spte->pagedir=thread_current()->pagedir;
  new_spte->type=PAGE_FILE;

  new_spte->file=file;
  new_spte->ofs=ofs;
  new_spte->read_bytes=read_bytes;
  new_spte->zero_bytes=zero_bytes;

  new_spte->present=false;
  new_spte->writable=writable;
  new_spte->dirty=false;

  new_spte->mmap=mmap;

  return new_spte;
}


void
s_page_table_destroy(struct hash* s_page_table)
{
  hash_destroy(s_page_table, free_spte);
}

/*
  Find and Return s-table entry whose vaddress is va.
  if there is no matched elem, return NULL.
*/
struct spte*
spte_find(struct thread* t, const void* va)
{ 
  struct spte* s_pte = malloc(sizeof(struct spte));
  s_pte->vaddress = pg_round_down(va);
  
  struct hash_elem* e = hash_find(t->s_page_table,&s_pte->elem);
  free(s_pte);
  
  if(e != NULL) return hash_entry(e, struct spte, elem);
  else return NULL;
}



/*
  Return hash value of virtual address of sup page table
*/
unsigned
s_page_table_hash(const struct hash_elem* he, void* aux UNUSED)
{
  struct spte *s_pte = hash_entry(he, struct spte, elem);
  return hash_int((int)s_pte->vaddress);
}

/*
  Compare virtual address of a and b
  if a's is smaller, return true.
  else, return false.
*/
bool
s_page_table_less_func(const struct hash_elem* a, const struct hash_elem* b, void* aux UNUSED)
{
  struct spte *spte_a = hash_entry(a, struct spte, elem);
  struct spte *spte_b = hash_entry(b, struct spte, elem);
  return spte_a->vaddress < spte_b->vaddress;
}

void
free_spte(struct hash_elem* he, void* aux UNUSED)
{
  struct spte* entry = hash_entry (he, struct spte, elem);
  if ( entry->type == PAGE_SWAP)
  {
    swap_free(entry->swap_idx);
  }
  free(entry);
  // printf("free complete\n");
}



/*
  Lazy Loading when frame not allocated yet.
  Find and load page for virtual address va.
  Return true at success, else exit(-1) 
*/
bool
load_s_page_table_entry(struct spte* spt_entry)
{
  if ( spt_entry == NULL )  exit(-1); 

  if ( spt_entry->present == true) return false;

  struct frame* new_frame = frame_allocate(PAL_USER, spt_entry);
  //struct file* new_file = spt_entry->file;

  // printf("\n>> Loading spt_entry->type: %d\n", spt_entry->type);
  switch(spt_entry->type)
  {
    case PAGE_FILE:
      // file_reopen(spt_entry->file);
      file_seek(spt_entry->file, spt_entry->ofs);
      //printf(">> %x, %x, %d\n", spt_entry->file, new_frame->kernel_virtual_address, spt_entry->read_bytes);
      if (file_read (spt_entry->file, new_frame->kernel_virtual_address, spt_entry->read_bytes) != (int)spt_entry->read_bytes)
      {
        frame_free(new_frame);
        exit(-1);
      }
      memset (new_frame->kernel_virtual_address+spt_entry->read_bytes, 0, spt_entry->zero_bytes);
      //file_close(new_file);
      break;
    case PAGE_ZERO:
      memset (new_frame->kernel_virtual_address, 0, PGSIZE);
      break;
    case PAGE_SWAP:
      swap_in (spt_entry->swap_idx, new_frame->kernel_virtual_address);
      // spt_entry->swap_idx = -1;
      // spt_etnry->
      // if (!)
      // {
      //   frame_free (new_frame);
      //   exit(-1);
      // };
      break;
    default:
      break;
  }

  /* Add the page to the process's address space. */
  if (!install_page (spt_entry->vaddress, new_frame->kernel_virtual_address, spt_entry->writable)) 
  {
    frame_free(new_frame);
    // printf("asdf\n");
    exit(-1);
    // printf("qwer\n");
  }

  spt_entry->pagedir=thread_current()->pagedir;
  pagedir_set_dirty(spt_entry->pagedir, new_frame->kernel_virtual_address, false);
  spt_entry->kaddress=(uint32_t*)new_frame->kernel_virtual_address;

  return true;
}


void update_spte_dirty(struct spte* spt_entry)
{
  if (spt_entry->kaddress == NULL) return;
    
  spt_entry->dirty = spt_entry->dirty
    || pagedir_is_dirty (spt_entry->pagedir, spt_entry->vaddress)
    || pagedir_is_dirty (spt_entry->pagedir, spt_entry->kaddress);

}

static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (
    pagedir_get_page (t->pagedir, upage) == NULL
    && pagedir_set_page (t->pagedir, upage, kpage, writable)
  );
}
