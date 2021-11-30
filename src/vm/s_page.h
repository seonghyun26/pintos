#ifndef __SPAGE
#define __SPAGE

#include <hash.h>
#include "threads/thread.h"
#include "filesys/file.h"

/* <--  Project 3 : VM S Page Table Start --> */
struct spte {
    uint32_t* vaddress;
    struct file* file;

    uint64_t access_time;
    bool present;
    bool write;
    bool dirty;

    struct hash_elem elem;
};

bool s_page_table_init(struct hash* s_page_table);
void s_page_table_destroy(struct hash* s_page_table);
void free_spte(struct hash_elem* he, void* aux UNUSED);

void s_page_table_entry_create(struct hash* s_page_table, struct spte* s);
void s_page_table_entry_delete(struct hash* s_page_table, struct spte* s);

unsigned s_page_table_hash(const struct hash_elem* he, void* aux UNUSED);
bool s_page_table_less_func(const struct hash_elem* a, const struct hash_elem* b, void* aux UNUSED);

struct spte* find_s_page_table(struct thread* t,void* va);
/* <--  Project 3 : VM S Page Table End --> */

#endif