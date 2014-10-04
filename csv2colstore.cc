#include <sys/stat.h>
#include <unistd.h>

#include "csvhelper.h"

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <csv_file> <colstore_name> <page_size>\n", argv[0]);
        return 1;
    }

    // read records from CSV file
    char* csv_file = argv[1];
    std::vector<Record*> records;
    int error = read_records(csv_file, &records);
    if (error) {
        fprintf(stderr, "Could not read records from file: %s\n", csv_file);
        return 2;
    }
    if (records.size() == 0) {
        fprintf(stderr, "No records contained in file: %s\n", csv_file);
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

    printf("opening attr files\n");

    // open all the attribute files for the column store
    FILE* attr_files[num_attributes];
    char attr_file_name[100];
    for (int i = 0; i < num_attributes; i++) {
        printf("filename: %s/%d\n", colstore_name, i);
        if (sprintf(attr_file_name, "%s/%d", colstore_name, i) < 0)  {
            fprintf(stderr, "Could not create attribute filename %s/%d\n", colstore_name, i);
            return 5;
        }

        printf("opening file %s\n", attr_file_name);

        FILE* attr_file = fopen(attr_file_name, "w+b");
        if (!attr_file) {
            fprintf(stderr, "Could not open attribute file for writing %s\n", attr_file_name);
        }

        attr_files[i] = attr_file;
    }

    printf("running through records\n");

    // run through the records and flush each attribute to its corresponding
    // file in the column store directory
    for (int i = 0; i < records.size(); i++) {
        Record* record = records[i];

        for (int j = 0; j < num_attributes; j++) {
            V attr_value = (*record)[j];
            FILE* attr_file = attr_files[j];

            fwrite(&i, sizeof(int), 1, attr_file);
            fwrite(&attr_value, sizeof(char), sizeof(attr_value), attr_file);
        }
    }

    // close all our files
    for (int i = 0; i < num_attributes; i++) {
        fclose(attr_files[i]);
    }

    return 0;
}