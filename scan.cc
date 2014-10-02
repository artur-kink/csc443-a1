#include "library.h"
#include "csvhelper.h"

/**
 * Print out all records in a heapfile.
 */
int main(int argc, char** argv){
    if(argc != 3){
        printf("Usage: scan <heap_file> <page_size>\n");
        return 1;
    }
    
    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    FILE* heap_file = fopen(argv[1], "rb");
    if(!heap_file){
        printf("Failed to open heap file: %s\n", argv[1]);
        return 2;
    }
    init_heapfile(heap, atoi(argv[2]), heap_file);
    
    RecordIterator* recordi = new RecordIterator(heap);
    while(recordi->hasNext()){
        Record next_record = recordi->next();
        print_record(&next_record);
    }

    fclose(heap_file);
    free(heap);
}
