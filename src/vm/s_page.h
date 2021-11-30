#include <hash.h>
#include "threads/thread.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include "vm/frame.h"

/* <--  Project 3 : VM S Page Table Start --> */
struct supplemental_page_table_entry {
    uint32_t* vaddress;
    struct frame* frame;
    struct file* file;

    uint64_t access_time;
    bool present;
    bool write;
    bool dirty;

    size_t offset

    struct hash_elem elem;
}

typedef struct supplemental_page_table_entry Spte;

void s_page_table_init(struct hash* s_page_table, hash_hash_func s_page_table_hash, hash_less_func s_page_table_less_func);
void s_page_table_destroy(struct hash* s_page_table);

void s_page_table_entry_create(struct hash* s_page_table, Spte* s);
void s_page_table_entry_delete(struct hash* s_page_table, Spte* s);

unsigned s_page_table_hash(struct hash_elem* he, void* aux UNUSED);
bool s_page_table_less_func(struct hash_elem* a, struct hash_elem* b, void* aux UNUSED);

Spte find_s_page_table(struct thread* t,void* va);
/* <--  Project 3 : VM S Page Table End --> */