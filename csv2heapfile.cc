#include "library.h"
#include "csvhelper.h"

#include <stdio.h>

void print_record(Record* record){
    for(int i = 0; i < record->size(); i++){
        printf("%.10s ", record->at(i));
    }
}

int main(int argc, char** argv){
    if(argc == 0)
        return 1;
    
    printf("Opening %s\n", argv[1]);
    
    std::vector<Record*> records;
    read_records(argv[1], &records);
    
    for(int i = 0; i < records.size(); i++){
        printf("Record %d: ", i);
        print_record(records.at(i));
        printf("\n");
        
        void* buf = malloc(record_size);
        fixed_len_write(records.at(i), buf);
        records[i] = new Record;
        fixed_len_read(buf, record_size, records.at(i));
        printf("Verification %d: ", i);
        print_record(records.at(i));
        printf("\n");
    }
    
}
