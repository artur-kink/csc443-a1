#include "library.h"
#include "csvhelper.h"

#include <stdio.h>

int main(int argc, char** argv) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <heapfile> <record_id> <attribute_id> <new_value> <page_size>", argv[0]);
        return 1;
    }

    // open file from first argument
    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    FILE* heap_file = fopen(argv[1], "r+b");
    if (!heap_file) {
        fprintf(stderr, "Failed to open heap file: %s\n", argv[1]);
        fclose(heap_file);
        free(heap);
        return 2;
    }

    // read in the record id
    int pid;
    int slot = parse_record_id(argv[2], &pid);
    if (slot == -1) {
        fprintf(stderr, "Invalid record id: %s\n", argv[2]);
        fclose(heap_file);
        free(heap);
        return 3;
    }

    // read in the rest of the arguments
    int attr_index = atoi(argv[3]);
    char* new_value = argv[4];
    int page_size = atoi(argv[5]);

    // initialize our heapfile; not using init_heapfile to avoid zeroing
    // out the directory
    heap->page_size = page_size;
    heap->file_ptr = heap_file;

    // read in the page to update
    Page* page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(page, page_size, record_size);

    if (try_read_page(heap, pid, page) == -1) {
        fprintf(stderr, "Page id out of bounds: %d\n", pid);
        fclose(heap_file);
        free(heap);
        free(page);
        return 4;
    }

    // make sure the record exists at the given slot
    if (is_freeslot(page, slot)) {
        fprintf(stderr, "Record with id %s does not exist\n", argv[2]);
        fclose(heap_file);
        free(heap);
        free(page);
        return 5;
    }

    // read in the record's contents, swap in the new attribute
    // value and write it back out to the page
    Record *record = new Record;
    read_fixed_len_page(page, slot, record);
    (*record)[attr_index] = new_value;
    write_fixed_len_page(page, slot, record);

    // write page back to the file
    write_page(page, heap, pid);

    // and free all our stuff
    fclose(heap_file);
    free(record);
    free(heap);

    return 0;
}
