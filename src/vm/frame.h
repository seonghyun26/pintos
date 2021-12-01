#ifndef __FRAME
#define __FRAME

#include <list.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "vm/s_page.h"

/* <--  Project 3 : VM Frame Table Start --> */
struct frame
{
  uint32_t* kernel_virtual_address; // physical address
  struct thread* thread;
  struct spte* spte;
  struct list_elem elem;
};

void frame_table_init(void);
struct frame* frame_allocate (enum palloc_flags p_flag, struct spte* spte);
void frame_free(struct frame* frame_to_deallocate);
struct frame* frame_evict (void);
struct frame* frame_find (uint32_t* kernel_virtual_address);
/* <--  Project 3 : VM Frame Table End --> */

#endif