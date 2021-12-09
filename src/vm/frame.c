#include "vm/frame.h"
#include "vm/s_page.h"
#include "vm/swap.h"
#include <list.h>
#include <stdio.h>
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "userprog/syscall.h"
#include <bitmap.h>

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
frame_allocate (enum palloc_flags p_flag, struct spte* spte)
{  
  ASSERT( spte != NULL );

  void* new_kernel_virtual_address = palloc_get_page(p_flag);
  // printf(">> In Frame.c : %x\n", new_kernel_virtual_address);
  if( new_kernel_virtual_address == NULL)
  {
    new_kernel_virtual_address = frame_evict(p_flag);
    if ( new_kernel_virtual_address == NULL )  {
      palloc_free_page(new_kernel_virtual_address);
      return NULL;
    }
  }
  // printf(">> Allocate Frame physical %x, va: %x\n",new_kernel_virtual_address, spte->vaddress);

  struct frame* new_frame = malloc(sizeof(struct frame));
  if ( new_frame == NULL ){
    palloc_free_page(new_kernel_virtual_address);
    return NULL;
  }
  new_frame->kernel_virtual_address = new_kernel_virtual_address;
  new_frame->thread = thread_current();
  new_frame->spte = spte;
  spte->kaddress = new_kernel_virtual_address;

  lock_acquire(&frame_table_lock);
  list_push_back(&frame_table, &new_frame->elem);
  lock_release(&frame_table_lock);

  // printf("Frame Allocated\n");
  return new_frame;
}

/*
  Free the given frame from the frame table
*/
void
frame_free (struct frame* frame_to_free)
{
  ASSERT ( frame_to_free != NULL );
  // printf(">> Free Frame %x\n", frame_to_free->kernel_virtual_address);
  
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
void*
frame_evict (enum palloc_flags p_flag)
{
  if ( list_empty(&frame_table) )
    PANIC ( "Frame Table Empty, Nothing to Evict" );

  // TODO: Evict a frame by LRU policy
  struct frame* frame_to_remove = list_entry(list_begin(&frame_table), struct frame , elem);
  struct spte* spt_entry = frame_to_remove->spte;
  size_t idx;

  // printf(" Evict Frame type: %d\n", spt_entry->type);
  switch(spt_entry->type)
  {
    case PAGE_FILE:
      if ( !spt_entry->writable ) break;
      // If file is from mmap
      if ( spt_entry->mmap )
      {
        update_spte_dirty(spt_entry);
        if ( spt_entry->dirty )
        {
          off_t byte = mmap_write_back(spt_entry->file, frame_to_remove->kernel_virtual_address, spt_entry->ofs);
          if ( byte == spt_entry->ofs ) return NULL;
        }
      }
      break;
    case PAGE_ZERO:
      update_spte_dirty(spt_entry);
      if (!spt_entry->dirty) break;
    case PAGE_SWAP:
      idx = swap_out (frame_to_remove->kernel_virtual_address);
      // printf("va:%x\n", frame_to_remove->spte->vaddress);
      if ( idx == BITMAP_ERROR )  return NULL;
      spt_entry->type = PAGE_SWAP;
      spt_entry->swap_idx = idx;
      break;
    default:
      NOT_REACHED();
      break;
  }
  frame_free(frame_to_remove);

  update_spte_dirty(spt_entry);


  void* new_kernel_virtual_address = palloc_get_page(p_flag);
  return new_kernel_virtual_address;
}

/*
  Find a frame with the given kernel virtual address
  in the frame table
*/
struct frame*
frame_find (uint8_t* kernel_virtual_address_cmp)
{
  struct frame* f;
  struct list_elem* e;

  for( e = list_begin(&frame_table) ; e != list_end(&frame_table) ; e = list_next(e) )
  {
    f = list_entry(e, struct frame, elem);
    if ( f->kernel_virtual_address == kernel_virtual_address_cmp )  return f;
  }
  return NULL;
}

struct frame*
frame_find_with_spte (struct spte* spt_entry)
{
  struct frame* f;
  struct list_elem* e;

  for( e = list_begin(&frame_table) ; e != list_end(&frame_table) ; e = list_next(e) )
  {
    f = list_entry(e, struct frame, elem);
    if ( f->spte == spt_entry )  return f;
  }
  return NULL;
}