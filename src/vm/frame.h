#ifndef __FRAME
#define __FRAME

#include <list.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/s_page.h"

/* <--  Project 3 : VM Frame Table Start --> */
struct frame
{
  uint8_t* kernel_virtual_address; // physical address
  struct thread* thread;
  struct spte* spte;
  struct list_elem elem;
};

void frame_table_init(void);
struct frame* frame_allocate (enum palloc_flags p_flag, struct spte* spte);
void frame_free(struct frame* frame_to_deallocate);
void* frame_evict (enum palloc_flags p_flag);
struct frame* frame_find (uint8_t* kernel_virtual_address);
struct frame* frame_find_with_spte(struct spte* spt_entry);
/* <--  Project 3 : VM Frame Table End --> */

#endif