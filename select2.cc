#include <string.h>
#include <sys/timeb.h>

#include "library.h"

int main(int argc, char** argv) {
    //Check for all arguments.
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <colstore_name> <attribute_id> <start> <end> <page_size>\n", argv[0]);
        return 1;
    }

    //Get select parameters.
    int attribute_id = atoi(argv[2]);
    char* start = argv[3];
    char* end = argv[4];

    //Open attribute file.
    char path[100] = "";
    strcat(path, argv[1]);
    strcat(path, "/");
    strcat(path, argv[2]);

    //Record Start Time
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;

    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    if (open_heapfile(heap, path, atoi(argv[5]), 2*attribute_len) != 0) {
        return 2;
    }

    RecordIterator* recordi = new RecordIterator(heap);

    //Find all records matching query.
    int number_of_records_matching_query = 0;
    int total_number_of_records = 0;
    while (recordi->hasNext()) {
        Record next_record = recordi->next();

        char attr[attribute_len+1];
        strncpy(attr, next_record.at(1), attribute_len);
        attr[attribute_len] = '\0';

        //Check if attribute in selection range.
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

    fclose(heap->file_ptr);
    free(heap);
    free(recordi);
    return 0;
}
