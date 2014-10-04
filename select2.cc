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

    //Create file name for attribute we're interested in.
    char filename[strlen(argv[1]) + strlen(argv[2]) + 1];
    if (sprintf(filename, "%s/%d", argv[1], attribute_id) < 0) {
        fprintf(stderr, "Could not make filename %s/%d\n", argv[1], attribute_id);
        return 2;
    }

    //Open attribute file.
    FILE* attr_file = fopen(filename, "r+b");
    if (!attr_file) {
        fprintf(stderr, "Failed to open attribute file: %s\n", filename);
        return 3;
    }

    //Record Start Time
    struct timeb t;
    ftime(&t);
    long start_ms = t.time * 1000 + t.millitm;

    //Find all records matching query.
    int number_of_records_matching_query = 0;
    int total_number_of_records = 0;
    while (!feof(attr_file)) {

        // skip over the tuple number since we don't need it
        fseek(attr_file, sizeof(int), SEEK_CUR);

        char* attr;
        fread(attr, 1, attribute_len, attr_file);

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

    fclose(attr_file);

    return 0;
}