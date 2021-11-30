#include <list.h>
#include "threads/thread.h"

/* <--  Project 3 : VM Frame Table Start --> */
struct frame
{
  uint32_t* kernel_virtual_address; // physical address
  struct thread* thread;
  struct list_elem elem;
};

void frame_table_init(void);
struct frame* frame_allocate (void);
void frame_free(struct frame* frame_to_deallocate);
struct frame* frame_evict (void);
struct frame* frame_find (struct frame* kernel_virtual_address);
/* <--  Project 3 : VM Frame Table End --> */