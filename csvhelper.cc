#include "csvhelper.h"

void read_records(const char* file, std::vector<Record*>* records){
    FILE* csvfile = fopen(file, "r");
    
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
            records->push_back(record);
        }
    }
}
