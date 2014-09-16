#include "library.h"

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
    
    FILE* csvfile = fopen(argv[1], "r");

    std::vector<Record*> records;
    
    while(!feof(csvfile)){
        Record* record = new Record;
        int i = 0;
        for(i = 0; i < num_attributes; i++){
            char* attribute = (char*)malloc(attribute_len);
            if(fread(attribute, attribute_len, 1, csvfile) == 0)
                break;
            fgetc(csvfile);
            record->push_back(attribute);
        }
        
        if(i == num_attributes){
            records.push_back(record);
        }
    }
    
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
