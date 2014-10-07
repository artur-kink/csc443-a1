#include "library.h"
#include "csvhelper.h"

#include <sys/stat.h>
#include <sys/timeb.h>
#include <stdio.h>
#include <stdlib.h>

int add_fixed_len_page_colstore(Page *page, Record *r, int attribute_id_to_write);

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

    // make column store directory if it doesn't already exist
    char* colstore_name = argv[2];
    struct stat st = {0};

    if (stat(colstore_name, &st) == -1) {
        if (mkdir(colstore_name, 0700) == -1) {
            fprintf(stderr, "Could not make column store directory: %s\n", colstore_name);
            return 4;
        }
    }

    //Record start time of program.
    //We do not include parsing of the csv because that is irrelevant to our metrics.
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;


    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));

    for (int j = 0; j < num_attributes; j++) {

        char path[100] = "";
        sprintf(path, "%s/%d", argv[2], j);

        //Open heap file where heap is stored.
        FILE* heap_file = fopen(path, "w+b");
        if(!heap_file){
            printf("Failed to open heap file to write to: %s\n", path);
            fclose(heap_file);
            free(heap);
            return 4;
        }

        init_heapfile(heap, atoi(argv[3]), heap_file);
        heap->slot_size = 2 * attribute_len;
        //Initialize first page.
        PageID page_id = alloc_page(heap);
        Page* page = (Page*)malloc(sizeof(Page));
        read_page(heap, page_id, page);

        //Loop all records and add them to heap.
        for(int i = 0; i < records.size(); i++){
            printf("Record %d, %d: %s\n", i, j, records.at(i)->at(j));
            // print_record(records.at(i));

            //If page is full, create new page in heap.
            if(add_fixed_len_page_colstore(page, records.at(i), j) == -1){

                printf("allocating new page\n");
                //Write page back to heap.
                write_page(page, heap, page_id);

                //Alloc new page and add record to it.
                page_id = alloc_page(heap);
                read_page(heap, page_id, page);
                add_fixed_len_page_colstore(page, records.at(i), j);

            }

//            Record r;
//            read_fixed_len_page(page, i % 8, &r);
//            printf("Result\n");
//            print_record(&r);
        }

        //Write our final page to heap.
        write_page(page, heap, page_id);
    }

    //Calculate program end time.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;
    printf("TIME: %lu\n", end_ms - start_ms);

    return 0;
}

int add_fixed_len_page_colstore(Page *page, Record *r, int attribute_id_to_write) {
    unsigned char* directory_offset = get_directory(page);

    //Iterate slots directory to find a free one.
    for(int i = 0; i < fixed_len_page_capacity(page); i++){
        if(i > 0 && i%8 == 0)
            directory_offset++;

        unsigned char directory = *directory_offset;

        if(directory >> (i%8) == 0) {
            Record to_write;

            char index[attribute_len];
            sprintf(index, "%0*d", attribute_len, attribute_id_to_write);
            to_write.push_back(index);
            to_write.push_back(r->at(attribute_id_to_write));

            //Write record to page.
            //fixed_len_write(&to_write, ((char*)page->data) + i*page->slot_size);

            char* buf = ((char*)page->data) + i*page->slot_size;

            // It was looping too many times so I tried this.
            for (int k = 0; k < 2; k++) {
                 memcpy((buf + k*attribute_len), (&to_write)->at(k), attribute_len);
            }

            //Update directory.
            directory |= 1 << (i%8);
            memcpy(directory_offset, &directory, 1);
            return i;
        }
    }

    //Reached here means we didn't find any free slots.
    return -1;
}
