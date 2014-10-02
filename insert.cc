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
    if (!heap_file) {
        printf("Failed to open heap file: %s\n", argv[1]);
        return 3;
    }

    init_heapfile(heap, atoi(argv[3]), heap_file);

    Page* p = (Page*)malloc(sizeof(Page*));
    Page* dp = (Page*)malloc(sizeof(Page*));

    PageID current_id = -1;
    int records_exhausted = 0;
    while (records_exhausted < records.size()) {

        // seek to the next free page, starting at current_id + 1 since we've
        // already operated on the page at current_id(or it's zero, in which case
        // we're at the first directory page and there's no need to examine it)
        PageID current_id = seek_page(p, dp, current_id + 1, heap, false);

        // if no free page exists, we need to create a new one to insert into.
        if (current_id == -1) {
            current_id = alloc_page(heap);
            read_page(heap, current_id, p);
        }

        // insert a record into each free slot, short-circuit if we run out
        std::vector<int> freeslots = fixed_len_page_freeslots(p);
        for (int i = 0; i < freeslots.size(); i++) {
            write_fixed_len_page(p, freeslots[i], records[records_exhausted]);
            records_exhausted++;

            if (records_exhausted >= records.size())
                break;
        }

        // only write the page if we would have emptied some records into it,
        // which occurs when there are some free record slots.
        if (freeslots.size() > 0)
            write_page(p, heap, current_id);
    }

    free(p);
    free(dp);

    fclose(heap_file);
    free(heap);

    return 0;
};
