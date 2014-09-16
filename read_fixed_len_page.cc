#include "library.h"
#include "csvhelper.h"

int main(int argc, char** argv){
    if(argc != 3){
        printf("Usage: read_fixed_len_page <page_file> <page_size>\n");
        return 1;
    }
    
    FILE* page_file = fopen(argv[1], "r");
    int page_size = atoi(argv[2]);
    
    while(!feof(page_file)){
        Page* page = new Page;
        init_fixed_len_page(page, page_size, record_size);
        
        if(fread(page->data, page_size, 1, page_file) == 0){
            break;
        }
        else{
            printf("Read page.\n");
            for(int i = 0; i < fixed_len_page_capacity(page); i++){
                Record record;
                read_fixed_len_page(page, i, &record);
                printf("Record %d: ", i);
                print_record(&record);
            }
        }
    }
    
}