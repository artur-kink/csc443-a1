#include "library.h"
#include "csvhelper.h"

#include <stdio.h>

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: %s <heapfile> <record_id> <page_size>\n", argv[0]);
        return 1;
    }

    // open file from first argument
    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    FILE* heap_file = fopen(argv[1], "r+b");
    if (!heap_file) {
        printf("Failed to open heap file: %s\n", argv[1]);
        fclose(heap_file);
        free(heap);
        return 3;
    }

    // read in rest of the arguments
    int record_id = atoi(argv[2]);
    int page_size = atoi(argv[3]);

    // initialize our heapfile and the page we'll be reading and writing to
    heap->page_size = page_size;
    heap->file_ptr = heap_file;

    Page* page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(page, page_size, record_size);

    // extract info from record id about what page and slot we're operating on
    int records_per_page = fixed_len_page_capacity(page);
    int pid = record_id / records_per_page;
    int slot = record_id % records_per_page;

    // read in that page and the record's contents, swap in the new attribute
    // value and write it back out
    read_page(heap, pid, page);

    //Get byte position of slot in the directory.
    unsigned char* directory_offset = ((unsigned char*)page->data) + fixed_len_page_directory_offset(page);
    printf("The offset delete found for pid %d is %d\n", pid, fixed_len_page_directory_offset(page));
    printf("The slot is %d \n", slot);
    directory_offset += slot/8;

    //Update directory, set as free.
    unsigned char directory = (unsigned char)*directory_offset;
    directory |= 0 << (slot%8);
    memcpy(directory_offset, &directory, 1);

    write_page(page, heap, pid);
    printf("the number of free slots: %ld\n", fixed_len_page_freeslots(page).size());

    // and free all our stuff
    fclose(heap_file);
    free(page);
    free(heap);
}
