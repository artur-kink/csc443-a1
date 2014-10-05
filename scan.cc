#include "library.h"
#include "csvhelper.h"

/**
 * Print out all records in a heapfile.
 */
int main(int argc, char** argv){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <heap_file> <page_size>\n", argv[0]);
        return 1;
    }

    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    FILE* heap_file = fopen(argv[1], "rb");
    if(!heap_file){
        fprintf(stderr, "Failed to open heap file: %s\n", argv[1]);
        free(heap);
        fclose(heap_file);
        return 2;
    }

    heap->page_size = atoi(argv[2]);
    heap->file_ptr = heap_file;

    RecordIterator* recordi = new RecordIterator(heap);
    while (recordi->hasNext()) {
        Record next_record = recordi->next();
        print_record(&next_record);
    }
}
