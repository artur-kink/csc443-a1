#include "library.h"
#include "csvhelper.h"

#include <string.h>
#include <sys/timeb.h>

int main(int argc, char** argv) {
    //Check for all arguments.
    if (argc != 6) {
        printf("Usage: select <heapfile> <attribute_id> <start> <end> <page_size>\n");
        return 1;
    }

    //Get select parameters.
    int attribute_id = atoi(argv[2]);
    char* start = argv[3];
    char* end = argv[4];
    
    FILE* heap_file = fopen(argv[1], "rb");
    if (!heap_file) {
        printf("Failed to open heap file: %s\n", argv[1]);
        return 2;
    }

    // Read in start and end
    Heapfile* heap = (Heapfile*) malloc(sizeof (Heapfile));
    init_heapfile(heap, atoi(argv[5]), heap_file);

    RecordIterator* recordi = new RecordIterator(heap);

    //Record Start Time 
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;

    //Find all records matching query.
    int number_of_records_matching_query = 0;
    int total_number_of_records = 0;
    while (recordi->hasNext()) {
        Record next_record = recordi->next();
        if(strcmp(next_record.at(attribute_id), start) >= 0 && strcmp(next_record.at(attribute_id), end) <= 0){
            printf("%.5s\n", next_record.at(attribute_id));
            total_number_of_records++;
        }
    }
    
    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;

    printf("TIME: %lu\n", end_ms - start_ms);
    printf("TOTAL NUMBER OF RECORDS : %d\n", total_number_of_records);
    printf("TOTAL NUMBER OF RECORDS SELECTED: %d\n", number_of_records_matching_query);

    fclose(heap_file);
    return 0;
}
