#include "vm/s_page.h"
#include <hash.h>
#include <stdlib.h>
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "threads/vaddr.h"

bool
s_page_table_init(struct hash* s_page_table)
{
  return hash_init(s_page_table, s_page_table_hash, s_page_table_less_func, 0);
}

void
s_page_table_destroy(struct hash* s_page_table)
{
  hash_destroy(s_page_table, free_spte);
}

void
free_spte(struct hash_elem* he, void* aux UNUSED)
{
  struct spte* entry = hash_entry (he, struct spte, elem);
  free(entry);
}

/*
  Create a new supplemental page table entry in the given hash table.
*/
void
s_page_table_entry_create(struct hash* s_page_table, struct spte* s)
{
  hash_insert(s_page_table, &s->elem);
}

/*
  Destory the given supplemental page table entry in
  the given supplemental page table.
*/
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

/*
  Find and Return s-table entry whose vaddress is va.
  if there is no matched elem, return NULL.
*/
struct spte*
find_s_page_table(struct thread* t,void* va)
{
  void* page_address = pg_round_down(va);
  
  struct spte* s_pte = (struct spte*)malloc(sizeof(struct spte*));
  s_pte->vaddress = page_address;
  
  struct hash_elem* e= hash_find(&t->s_page_table,&s_pte->elem);
  free(s_pte);
  
  if(e!=NULL) return hash_entry(e, struct spte, elem);
  else return NULL;
}