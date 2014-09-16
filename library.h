#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef const char* V;
typedef std::vector<V> Record;

typedef struct {
    void *data;
    int page_size;
    int slot_size;
} Page;

typedef struct {
    FILE *file_ptr;
    int page_size;
} Heapfile;

#define num_attributes 100
#define attribute_len 10
#define record_size num_attributes*attribute_len

/**
 * Compute the number of bytes required to serialize record
 */
int fixed_len_sizeof(Record *record);

/**
 * Serialize the record to a byte array to be stored in buf.
 */
void fixed_len_write(Record *record, void *buf);

/**
 * Deserializes `size` bytes from the buffer, `buf`, and
 * stores the record in `record`.
 */
void fixed_len_read(void *buf, int size, Record *record);

/**
 * Initializes a page using the given slot size
 */
void init_fixed_len_page(Page *page, int page_size, int slot_size);
 
/**
 * Calculates the maximal number of records that fit in a page
 */
int fixed_len_page_capacity(Page *page);
  
/*
 * Calculate the free space (number of free slots) in the page
 */
int fixed_len_page_freeslots(Page *page);
   
/**
 * Add a record to the page
 * Returns:
 * record slot offset if successful,
 * -1 if unsuccessful (page full)
 */
int add_fixed_len_page(Page *page, Record *r);
    
/**
 * Write a record into a given slot.
 */
void write_fixed_len_page(Page *page, int slot, Record *r);
     
/**
 * Read a record from the page from a given slot.
 */
void read_fixed_len_page(Page *page, int slot, Record *r);
