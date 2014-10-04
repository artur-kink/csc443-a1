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

    // read page size
    int page_size = atoi(argv[3]);

    // initialize our heapfile
    heap->page_size = page_size;
    heap->file_ptr = heap_file;

    // initialize the page we'll be reading and writing to
    Page* page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(page, page_size, record_size);

    if (try_read_page(heap, pid, page) == -1) {
        printf("Page id out of bounds: %d\n", pid);
        free(page);
        fclose(heap_file);
        free(heap);
        return 4;
    }

    //Get byte position of slot in the directory.
    unsigned char* directory_offset = get_directory(page);
    directory_offset += slot/8;

    //Update directory, set as free.
    unsigned char directory = (unsigned char)*directory_offset;
    directory &= ~(1 << (slot%8));
    memcpy(directory_offset, &directory, 1);

    write_page(page, heap, pid);

    // and free all our stuff
    fclose(heap_file);
    free(page);
    free(heap);

    return 0;
}
