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
    
    Page* page = (Page*)malloc(sizeof(Page));
    read_page(heap, 0, page);
    
    unsigned char* directory_offset = ((unsigned char*)page->data) + fixed_len_page_directory_offset(page);
            
    int record_count = 0;

    //Traverse all records in read page and print them.
    for(int i = 0; i < fixed_len_page_capacity(page); i++){
        if(i > 0 && i%8 == 0){
            directory_offset += 1;
        }

        //Retrieve and print record from slot if its marked in the directory.
        unsigned char directory = (unsigned char)*directory_offset;
        if(directory >> (i%8) & 0x01){
            Record record;
            read_fixed_len_page(page, i, &record);
            printf("Record %d at slot %d: ", record_count, i);
            print_record(&record);
            record_count++;
        }

    }
}
