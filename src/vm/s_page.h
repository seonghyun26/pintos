#ifndef __SPAGE
#define __SPAGE

#include <hash.h>
#include "threads/thread.h"
#include "filesys/file.h"

/* <--  Project 3 : VM S Page Table Start --> */
enum page_type
{
    PAGE_FILE,
    PAGE_ZERO
};

struct spte {
    uint32_t* vaddress;     // virtual memory address
    enum page_type type;    // page type declared using enum above

    // Information needed for Lazy Loading
    struct file* file;
    off_t ofs;
    size_t read_bytes;
    size_t zero_bytes;

    // Information about page
    uint64_t access_time;
    bool present;
    bool writable;
    bool dirty;

    struct hash_elem elem;
};

struct hash* s_page_table_init(void);
void s_page_table_destroy(struct hash* s_page_table);
void s_page_table_entry_insert(struct hash* s_page_table, struct spte* s);
void s_page_table_entry_delete(struct hash* s_page_table, struct spte* s);

unsigned s_page_table_hash(const struct hash_elem* he, void* aux UNUSED);
bool s_page_table_less_func(const struct hash_elem* a, const struct hash_elem* b, void* aux UNUSED);
void free_spte(struct hash_elem* he, void* aux UNUSED);

struct spte* find_s_page_table(struct thread* t, const void* va);

bool load_s_page_table_entry(void* va);
/* <--  Project 3 : VM S Page Table End --> */

#endif