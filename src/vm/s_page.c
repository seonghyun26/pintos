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
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"

static bool install_page (void *upage, void *kpage, bool writable);


struct hash*
s_page_table_init(void)
{
  struct hash* new_s_page_table = malloc(sizeof(struct hash));
  hash_init(new_s_page_table, s_page_table_hash, s_page_table_less_func, 0);

  return new_s_page_table;
}

void
s_page_table_destroy(struct hash* s_page_table)
{
  hash_destroy(s_page_table, free_spte);
}

void
s_page_table_entry_insert(struct hash* s_page_table, struct spte* s)
{
  // printf("\n---A FLAG---\n");
  hash_insert(s_page_table, &s->elem);
  // printf("\n---B FLAG---\n");
}

void
s_page_table_entry_delete(struct hash* s_page_table, struct spte* s)
{
  hash_delete(s_page_table, &s->elem);
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
  free(entry);
}




/*
  Find and Return s-table entry whose vaddress is va.
  if there is no matched elem, return NULL.
*/
struct spte*
find_s_page_table(struct thread* t, const void* va)
{ 
  struct spte* s_pte = malloc(sizeof(struct spte));
  s_pte->vaddress = pg_round_down(va);
  
  struct hash_elem* e = hash_find(t->s_page_table,&s_pte->elem);
  free(s_pte);
  
  if(e != NULL) return hash_entry(e, struct spte, elem);
  else return NULL;
}


bool
load_s_page_table_entry(void* va)
{
  //Lazy Loading when frame not allocated
  
  //Find S-page table for page falut address
  struct spte* spt_entry = find_s_page_table(thread_current(), pg_round_down(va));
  if ( spt_entry == NULL )  exit(-1);
  
  if ( spt_entry->present == 1) return false;
  else {
    struct frame* new_frame = frame_allocate(PAL_USER, spt_entry);
    //struct file* new_file = spt_entry->file;

    // file_reopen(spt_entry->file);
    file_seek(spt_entry->file, spt_entry->ofs);
    //printf(">> %x, %x, %d\n", spt_entry->file, new_frame->kernel_virtual_address, spt_entry->read_bytes);
    if (file_read (spt_entry->file, new_frame->kernel_virtual_address, spt_entry->read_bytes) != (int)spt_entry->read_bytes)
    {
      frame_free(new_frame);
      exit(-1);
    }
    memset (new_frame->kernel_virtual_address + spt_entry->read_bytes, 0, spt_entry->zero_bytes);

    // /* Add the page to the process's address space. */
    if (!install_page (spt_entry->vaddress, new_frame->kernel_virtual_address, spt_entry->writable)) 
    {
      frame_free(new_frame);
      exit(-1);
    }
    //file_close(new_file);
  }
  return true;
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
