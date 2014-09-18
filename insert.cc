#include "library.h"
#include "csvhelper.h"

/**
 * Insert records from a csv into an existing heap file.
 */
int main(int argc, char** argv){
    if(argc != 4){
        printf("Usage: insert <heapfile> <csv_file> <page_size>\n");
        return 1;
    }
    
    //Load records from csv.
    std::vector<Record*> records;
    read_records(argv[2], &records);
    if(records.size() == 0){
        printf("No records in csv.\n");
        return 2;
    }
    
    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    FILE* heap_file = fopen(argv[1], "r+b");
    if(!heap_file){
        printf("Failed to open heap file: %s\n", argv[1]);
        return 3;
    }
    init_heapfile(heap, atoi(argv[3]), heap_file);
    
};
