#include "vm/swap.h"
#include <bitmap.h>
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <stdio.h>
#include <debug.h>

#define BLOCKS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

struct block *swap_block;
struct lock swap_lock;
struct bitmap* swap_bitmap;
size_t swap_size;


void
swap_init()
{
  lock_init (&swap_lock);

  // Create a new block for swap, set size of it
  swap_block = block_get_role(BLOCK_SWAP);
  swap_size = block_size(swap_block) / BLOCKS_PER_PAGE;
  // printf(">> PGSIZE %d, BLOCK_SECTOR_SIZE %d, BLOCKS_PER_PAGE %d\n", PGSIZE, BLOCK_SECTOR_SIZE,BLOCKS_PER_PAGE);
  
  // Create a bitmap with size swap_size, and set lit all true
  swap_bitmap = bitmap_create(swap_size);
  bitmap_set_all(swap_bitmap, true);
  // printf(">> Swap Init Complete\n");
}

/*
  From Swap disk to Physcial Memory
*/
void 
swap_in(size_t idx, void* va)
{
  // printf(">> Swap In\n");
  lock_acquire(&swap_lock);
  if ( bitmap_test(swap_bitmap, idx) == true )
  {
    lock_release(&swap_lock);
    printf("Available Slot\n");
    return;
  }

  size_t i;
  for ( i = 0 ; i < BLOCKS_PER_PAGE ; i++ ){
    block_read(
      swap_block,
      i + idx * BLOCKS_PER_PAGE,
      va + ( i *BLOCK_SECTOR_SIZE)
    );
  }

  bitmap_set(swap_bitmap, idx, true);
  lock_release(&swap_lock);

  // printf(">> Swap In Complete\n");

  return;
}

/*
  From Physcial Memory to Swap disk
*/
size_t
swap_out(void* va)
{
  // printf(">> Swap Out from %x\n", va);
  lock_acquire(&swap_lock);

  size_t idx = bitmap_scan(swap_bitmap, 0, 1, true);
  if(idx == BITMAP_ERROR)
  {
    lock_release(&swap_lock);
    return BITMAP_ERROR;
  }

  block_sector_t sector = BLOCKS_PER_PAGE * idx;
  size_t i;
  for(i = 0; i < BLOCKS_PER_PAGE; i++)
  {
    block_write (
      swap_block,
      sector + i,
      va + i * BLOCK_SECTOR_SIZE 
    );
  }

  // block_print_stats();
  bitmap_set(swap_bitmap, idx, false); 

  lock_release(&swap_lock);
  // printf(">> Swap Out Complete\n");

  return idx;
}

void
swap_free(size_t idx)
{
  // printf(">> Swap Free\n");
  lock_acquire (&swap_lock);
  bitmap_set (swap_bitmap, idx, true);
  lock_release (&swap_lock);
  // printf(">> Swap Free Complete\n");
  
  return;
}