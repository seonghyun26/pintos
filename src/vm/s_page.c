#include "vm/s_page.h"
#include <list.h>
#include <hash.h>

void
s_page_table_init(
  struct hash* s_page_table,
  hash_hash_func s_page_table_hash,
  hash_less_func s_page_table_less_func
)
{
  hash_init( s_page_table, s_page_table_hash,s_page_table_less_func);
}

void
s_page_table_destroy(struct hash* s_page_table)
{
  hash_destroy(s_page_table);
}

/*
  Create a new supplemental page table entry in the given hash table.
*/
bool
s_page_table_entry_create(struct hash* s_page_table, Spte* s)
{
  hash_insert(&s_page_table, new_Spte);
}

/*
  Destory the given supplemental page table entry in
  the given supplemental page table.
*/
void
s_page_table_entry_delete(struct hash* s_page_table, Spte* s)
{
  hash_delete(&s_page_table, s->elem);
}


/*
  Return hash value of virtual address of sup page table
*/
unsigned
s_page_table_hash(struct hash_elem* he, void* aux UNUSED)
{
  Spte *spte = list_entry(he,Spte,elem);
  return hash_int(se->vaddress);
}


/*
  Compare virtual address of a and b
  if a's is smaller, return true.
  else, return false.
*/
bool
s_page_table_less_func(struct hash_elem* a, struct hash_elem* b, void* aux UNUSED)
{
  Spte *spte_a = list_entry(a,Spte,elem);
  Spte *spte_b = list_entry(b,Spte,elem);
  return spte_a->vaddress < spte_b->vaddress;
}

/*
  Find and Return s-table entry whose vaddress is va.
  if there is no matched elem, return NULL.
*/
Spte 
find_s_page_table(struct thread* t,void* va)
{
  void* page_address = pg_round_down(va);
  
  Spte* spte = (Spte*)malloc(sizeof(Spte*));
  spte->vaddress = page_address;
  
  struct hash_elem* e= hash_find(t->s_page_table,&spte->hash_elem);
  free(spte);
  
  if(e!=NULL) return list_entry(e,Spte,elem);
  else return NULL;
}