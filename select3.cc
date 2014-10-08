#include <string.h>
#include <sys/timeb.h>

#include "library.h"

int main(int argc, char** argv) {
    //Check for all arguments.
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <colstore_name> <attribute_id> <return_attribute_id> <start> <end> <page_size>\n", argv[0]);
        return 1;
    }

    //Get select parameters.
    int attribute_id = atoi(argv[2]);
    int return_attribute_id = atoi(argv[2]);
    char* start = argv[4];
    char* end = argv[5];

    //Open attribute file.
    char path[100] = "";
    strcat(path, argv[1]);
    strcat(path, "/");
    strcat(path, argv[2]);
    FILE* attr_file = fopen(path, "r+b");
    if (!attr_file) {
        fprintf(stderr, "Failed to open attribute file: %s\n", path);
        return 3;
    }

    //
    //Record Start Time
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;

    //Initialize heap and record iterator from file.
    Heapfile* heap = (Heapfile*) malloc(sizeof (Heapfile));
    heap->page_size = atoi(argv[6]);
    heap->slot_size = 2*attribute_len;
    heap->file_ptr = attr_file;
    RecordIterator* recordi = new RecordIterator(heap);

    //Find all records matching query.
    std::vector<int> matching_records;
    int number_of_records_matching_query = 0;
    int total_number_of_records = 0;
    while (recordi->hasNext()) {
        Record next_record = recordi->next();

        //Check if attribute in selection range.
        if(strcmp(next_record.at(1), start) >= 0 && strcmp(next_record.at(1), end) <= 0){
            matching_records.push_back(atoi(next_record.at(0)));
            number_of_records_matching_query++;
        }
        total_number_of_records++;
    }
    fclose(attr_file);
    //Close the other heap and open this one. Get the matching records.

    //Open return attribute file.
    char second_path[100] = "";
    strcat(second_path, argv[1]);
    strcat(second_path, "/");
    strcat(second_path, argv[3]);

    attr_file = fopen(second_path, "r+b");
    if (!attr_file) {
        fprintf(stderr, "Failed to open return attribute file: %s\n", second_path);
        return 3;
    }
    heap->file_ptr = attr_file;

    // read in the page to update
    Page* page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(page, heap->page_size, heap->slot_size);

    int pid = -1;
    for (int i = 0; i < matching_records.size(); i++) {
        if (pid != matching_records[i] / fixed_len_page_capacity(page)) {
            pid = matching_records[i] / fixed_len_page_capacity(page);
            if (try_read_page(heap, pid, page) == -1) {
                fprintf(stderr, "Page id out of bounds: %d\n", pid);
                break;
            }
        }

        int slot = matching_records[i] % fixed_len_page_capacity(page);
        // make sure the record exists at the given slot
        if (is_freeslot(page, slot)) {
            fprintf(stderr, "Record with id %s does not exist\n", argv[2]);
            break;
        }
        // read in the record's contents, swap in the new attribute
        // value and write it back out to the page
        Record *record = new Record;
        read_fixed_len_page(page, slot, record);
        printf("%.5s\n", (*record)[1]);
    }

    //Calculate program runtime.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;

    //Print metrics.
    printf("TIME: %lu\n", end_ms - start_ms);
    printf("TOTAL NUMBER OF RECORDS : %d\n", total_number_of_records);
    printf("TOTAL NUMBER OF RECORDS SELECTED: %d\n", number_of_records_matching_query);

    fclose(attr_file);
    free(heap);
    free(recordi);
    return 0;
}
