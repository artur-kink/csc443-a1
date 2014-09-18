#include "library.h"
#include "csvhelper.h"

#include <sys/timeb.h>
#include <stdio.h>

int main(int argc, char** argv){
    if(argc != 4){
        printf("Usage: csv2heapfile <csv_file> <heapfile> <page_size>\n");
        return 1;
    }
    
    printf("Opening %s\n", argv[1]);
    
    std::vector<Record*> records;
    read_records(argv[1], &records);
    
    //Record start time of program.
    //We do not include parsing of the csv because that is irrelevant to our metrics.
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;
    
    printf("Initializing Heapfile\n");
    
    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    FILE* heap_file = fopen(argv[2], "w+b");
    init_heapfile(heap, atoi(argv[3]), heap_file);
    
    printf("Initializing Page\n");
    
    PageID page_id = alloc_page(heap);
    Page* page = (Page*)malloc(sizeof(Page));
    read_page(heap, page_id, page);
    
    printf("Adding records to Page\n");
    
    for(int i = 0; i < records.size(); i++){
        printf("Record %d: ", i);
        print_record(records.at(i));
        
        if(add_fixed_len_page(page, records.at(i)) == -1){
            //Write page back to heap.
            write_page(page, heap, page_id);
            
            //Alloc new page and add record to it.
            page_id = alloc_page(heap);
            read_page(heap, page_id, page);
            add_fixed_len_page(page, records.at(i));
        }
    }
    write_page(page, heap, page_id);
    
    //Calculate program end time.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;
    printf("TIME: %d\n", end_ms - start_ms);
}