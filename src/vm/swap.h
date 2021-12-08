#ifndef __SWAP
#define __SWAP

#include <stdio.h>

void swap_init(void);
void swap_in(size_t idx, void* va);
size_t swap_out(void* va);
void swap_free(size_t idx);

#endif