#include "vm/frame.h"
#include <list.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"

static struct list frame_table;
static struct lock frame_table_lock;

/* Initalize Frame Table */
void
frame_table_init ()
{
  list_init(&frame_table);
  lock_init(&frame_table_lock);
}

/*
  Allocated new frame for a page using palloc_get_page from p_flag
  If Fail, evcit a frame and try one more
  If Success, Make a new frame_entry and pushback in frame table
*/
struct frame*
frame_allocate (enum palloc_flags p_flag)
{
  lock_acquire(&frame_table_lock);
  void* kernel_virtual_address_ = palloc_get_page(p_flag);
  if( kernel_virtual_address_ == NULL)
  {
    kernel_virtual_address_ = frame_evict();
    if ( kernel_virtual_address_ == NULL )
    {
      lock_release(&frame_table_lock);
      return NULL;
    }
  }

  struct frame* new_frame = (struct frame*)malloc(sizeof(struct frame));
  ASSERT ( new_frame != NULL );
  new_frame->kernel_virtual_address = kernel_virtual_address_;
  new_frame->thread = thread_current();

  list_push_back(&frame_table, &new_frame->elem);

  lock_release(&frame_table_lock);

  return new_frame;
}

/*
  Free the given frame from the frame table
*/
void
frame_free (struct frame* frame_to_free)
{
  ASSERT ( frame_to_free != NULL );

  lock_acquire(&frame_table_lock);

  list_remove(&frame_to_free->elem);
  palloc_free_page(frame_to_free->kernel_virtual_address); 
  free(frame_to_free);

  lock_release(&frame_table_lock);
}


/*
  Choose a frame to evict using LRU policy
  Evict the frame, and return the kernel virtual adress
*/
struct frame*
frame_evict ()
{
  if ( !list_empty(&frame_table) )
    PANIC ( "Frame Table Empty, Nothing to Evict" );

  // TODO: Evict a frame by LRU policy
  struct frame *frame = NULL;
  // struct hash* s_page_table = frame->thread->s_page_table;

  return frame;
}

/*
  Find a frame with the given kernel virtual address
  in the frame table
*/
struct frame*
frame_find (uint32_t* kernel_virtual_address_)
{
  struct frame* f;
  struct list_elem* e;

  for( e = list_begin(&frame_table) ; e != list_end(&frame_table) ; e = list_next(e) )
  {
    f = list_entry(e, struct frame, elem);
    if ( f->kernel_virtual_address == kernel_virtual_address_ )  return f;
  }
  return NULL;
}