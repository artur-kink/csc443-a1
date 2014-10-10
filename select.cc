#include "library.h"
#include "csvhelper.h"

#include <string.h>
#include <sys/timeb.h>

/**
 * Select a substring(first five letters) of a given attribute, <attribute_id> such
 * that the attribute >= <start> and attribute <= <end>.
 */
int main(int argc, char** argv) {
    //Check for all arguments.
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <heapfile> <attribute_id> <start> <end> <page_size>\n", argv[0]);
        return 1;
    }

    //Get select parameters.
    int attribute_id = atoi(argv[2]);
    char* start = argv[3];
    char* end = argv[4];

    //Open heap file.
    FILE* heap_file = fopen(argv[1], "rb");
    if (!heap_file) {
        fprintf(stderr, "Failed to open heap file: %s\n", argv[1]);
        return 2;
    }

    //Record Start Time
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;

    //Initialize heap and record iterator from file.
    Heapfile* heap = (Heapfile*) malloc(sizeof (Heapfile));
    heap->page_size = atoi(argv[5]);
    heap->slot_size = record_size;
    heap->file_ptr = heap_file;
    RecordIterator* recordi = new RecordIterator(heap);

    //Find all records matching query.
    int number_of_records_matching_query = 0;
    int total_number_of_records = 0;
    while (recordi->hasNext()) {
        Record next_record = recordi->next();

        //Check if attribute in selection range.
        char attr[attribute_len+1];
        strncpy(attr, next_record.at(attribute_id), attribute_len);
        attr[attribute_len] = '\0';

        if(strcmp(attr, start) >= 0 && strcmp(attr, end) <= 0){
            printf("%.5s\n", attr);
            number_of_records_matching_query++;
        }
        total_number_of_records++;
    }

    //Calculate program runtime.
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;

    //Print metrics.
    printf("TIME: %lu\n", end_ms - start_ms);
    printf("TOTAL NUMBER OF RECORDS : %d\n", total_number_of_records);
    printf("TOTAL NUMBER OF RECORDS SELECTED: %d\n", number_of_records_matching_query);

    fclose(heap_file);
    free(heap);
    free(recordi);
    return 0;
}
