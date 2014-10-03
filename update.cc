#include "library.h"
#include "csvhelper.h"

#include <stdio.h>

int main(int argc, char** argv) {
    if (argc != 6) {
        printf("Usage: %s <heapfile> <record_id> <attribute_id> <new_value> <page_size>", argv[0]);
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
    int attr_index = atoi(argv[3]);
    char* new_value = argv[4];
    int page_size = atoi(argv[5]);

    // initialize our heapfile and the page we'll be reading and writing to
    init_heapfile(heap, page_size, heap_file);
    Page* page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(page, page_size, record_size);

    // extract info from record id about what page and slot we're operating on
    int records_per_page = fixed_len_page_capacity(page);
    int pid = record_id / records_per_page;
    int slot = record_id % records_per_page;

    // read in that page and the record's contents, swap in the new attribute
    // value and write it back out
    read_page(heap, pid, page);
    read_fixed_len_page(page, slot, r);
    (*r)[attr_index] = new_value;
    write_fixed_len_page(page, slot, r);

    // and free all our stuff
    fclose(heap_file);
    free(r);
    free(heap);
}
