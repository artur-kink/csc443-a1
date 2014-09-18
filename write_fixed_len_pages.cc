#include "library.h"
#include "csvhelper.h"
#include <sys/timeb.h>

/**
 * Takes a csv file, reads all the values into records.
 * The records are all added to pages of size page_size.
 * Pages are written to page_file as they fill up.
 */
int main(int argc, char** argv){
    
    //Make sure all args given.
    if(argc != 4){
        printf("Usage: write_fixed_len_pages <csv_file> <page_file> <page_size>\n");
        return 1;
    }
    
    //Open page file.
    FILE* page_file = fopen(argv[2], "w+b");
    if(!page_file){
        printf("Failed to open page file: %s\n", argv[2]);
        return 2;
    }
    
    int page_size = atoi(argv[3]);
    
    //Get records
    std::vector<Record*> records;
    read_records(argv[1], &records);
   
    //Record start time of program.
    //We do not include parsing of the csv because that is irrelevant to our metrics.
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;
    
    //Create initial page
    Page* page = (Page*)malloc(sizeof(Page));;
    init_fixed_len_page(page, page_size, record_size);
    int page_counter = 1;
    
    //Add records to pages
    for(int i = 0; i < records.size(); i++){
        
        if(add_fixed_len_page(page, records.at(i)) == -1){
            //Write page to file.
            fwrite(page->data, 1, page->page_size, page_file);
            fflush(page_file);
            
            //Create new page.
            free_fixed_len_page(page);
            init_fixed_len_page(page, page_size, record_size);
            add_fixed_len_page(page, records.at(i));
            page_counter++;
        }
    }
    
    //Write final page to file.
    fwrite(page->data, 1, page->page_size, page_file);
    fflush(page_file);
    fclose(page_file);
    
    //Release page memory.
    free_fixed_len_page(page);
    free(page);
    
    //Calculate program end time.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;
    
    //Print metrics.
    printf("NUMBER OF RECORDS: %d\n", records.size());
    printf("NUMBER OF PAGES: %d\n", page_counter);
    printf("TIME: %d\n", end_ms - start_ms);
}