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
        fclose(heap_file);
        free(heap);
        return 3;
    }

    //init_heapfile(heap, atoi(argv[3]), heap_file);

    heap->page_size = atoi(argv[3]);
    heap->file_ptr = heap_file;

    Page* page = (Page*)malloc(sizeof(Page*));
    Page* directory_page = (Page*)malloc(sizeof(Page*));

    PageID current_id = 0; // we increment at the top of the loop;
    int records_exhausted = 0;
    while (records_exhausted < records.size()) {
        current_id = seek_page(page, directory_page, current_id, heap, false);

        // if no free page exists, we need to create a new one to insert into.
        if (current_id == -1) {
            current_id = alloc_page(heap);
            read_page(heap, current_id, page);
        }

        // insert a record into each free slot, short-circuit if we run out
        std::vector<int> freeslots = fixed_len_page_freeslot_indices(page);
        for (int i = 0; i < freeslots.size(); i++) {
            write_fixed_len_page(page, freeslots[i], records[records_exhausted]);
            records_exhausted++;

            if (records_exhausted >= records.size())
                break;
        }

        // only write the page if we would have emptied some records into it,
        // which occurs when there are some free record slots.
        if (freeslots.size() > 0) {
            write_page(page, heap, current_id);
        }
    }

    free(page);
    free(directory_page);

    fclose(heap_file);
    free(heap);

    return 0;
};
