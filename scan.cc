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
    if (open_heapfile(heap, argv[1], atoi(argv[2]), record_size) != 0) {
        return 2;
    }

    RecordIterator* recordi = new RecordIterator(heap);
    while (recordi->hasNext()) {
        Record next_record = recordi->next();
        recordi->printRecords(&next_record);
    }

    fclose(heap->file_ptr);
    free(heap);
    free(recordi);

    return 0;
}
