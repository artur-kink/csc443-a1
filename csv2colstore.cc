#include "library.h"
#include "csvhelper.h"

#include <sys/timeb.h>
#include <stdio.h>

/**
 * Takes a csv file and converts it to a heap file with given page sizes.
 */
int main(int argc, char** argv){
    //Make sure all args are provided.
    if(argc != 4){
        fprintf(stderr, "Usage: %s <csv_file> <heapfile> <page_size>\n", argv[0]);
        return 1;
    }

    //Load records from csv.
    std::vector<Record*> records;
    int error = read_records(argv[1], &records);
    if (error) {
        fprintf(stderr, "Could not read records from file: %s\n", argv[1]);
        return 2;
    }

    if(records.size() == 0){
        fprintf(stderr, "No records in file: %s\n", argv[1]);
        return 3;
    }

    //Record start time of program.
    //We do not include parsing of the csv because that is irrelevant to our metrics.
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;

    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    //Open heap file where heap is stored.
    FILE* heap_file = fopen(argv[2], "w+b");
    if(!heap_file){
        printf("Failed to open heap file to write to: %s\n", argv[2]);
        fclose(heap_file);
        free(heap);
        return 4;
    }
    init_heapfile(heap, atoi(argv[3]), heap_file);

    //Initialize first page.
    PageID page_id = alloc_page(heap);
    Page* page = (Page*)malloc(sizeof(Page));
    read_page(heap, page_id, page);

    //Loop all records and add them to heap.
    for(int i = 0; i < records.size(); i++){
        printf("Record %d: ", i);
        print_record(records.at(i));

        //If page is full, create new page in heap.
        if(add_fixed_len_page(page, records.at(i)) == -1){

            //Write page back to heap.
            write_page(page, heap, page_id);

            //Alloc new page and add record to it.
            page_id = alloc_page(heap);
            read_page(heap, page_id, page);
            add_fixed_len_page(page, records.at(i));
        }
    }

    //Write our final page to heap.
    write_page(page, heap, page_id);

    //Calculate program end time.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;
    printf("TIME: %lu\n", end_ms - start_ms);

    return 0;
}

void fixed_len_write(Record *record, void *buf) {
    int int_len = sizeof(int);
    memcpy(((char*)buf), record->at(0), int_len);
    memcpy(((char*)buf + int_len), record->at(1), attribute_len);
}

void fixed_len_read(void *buf, int size, Record *record) {
    int int_len = sizeof(int);
    V record_id = (char *) buf;
    record->push_back(record_id);
    V attr = (char *) buf + int_len;
    record->push_back(attr);
}
