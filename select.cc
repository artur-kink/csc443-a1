#include "library.h"
#include <sys/timeb.h>

int main(int argc, char** argv) {
    // SELECT SUBSTRING(A, 1, 5) FROM T
    // WHERE A >= start AND A <= end

    if (argc != 5){
        printf( "Must put <heapfile> <attribute_id> <start> <end> <page_size>\n" );
        return 1;
    }

    FILE* heap_file = fopen(argv[1], "rb");
    if(!heap_file){
        printf("Failed to open page file: %s\n", argv[1]);
        return 2;
    }

    // Read in start and end
    Heapfile* heap = (Heapfile*)malloc(sizeof(Heapfile));
    init_heapfile(heap, atoi(argv[5]), heap_file);
    
    RecordIterator* recordi = new RecordIterator(heap);

    //Record Start Time 
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;


    int number_of_records_matching_query = 0;
    int total_number_of_records = 0;
    while(recordi->hasNext()){
        Record next_record = recordi->next();
        // WHERE A >= start AND A <= end
        // TODO: Figure this out.
        
        // The actual content

        total_number_of_records++;
    }
    fclose(heap_file);

    ftime(&t);
    long end_ms = t.time * 1000 + t.millitm;
    printf("TIME: %lu\n", end_ms - start_ms);

    printf("TOTAL NUMBER OF RECORDS : %d\n", total_number_of_records);
    printf("TOTAL NUMBER OF RECORDS SELECTED: %d\n", number_of_records_matching_query);

    return 0;
}