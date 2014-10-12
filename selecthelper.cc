#include <string.h>
#include <sys/timeb.h>

#include "library.h"

int compare_record(char* attr, const char* record_data, char* start, char* end){
    strncpy(attr, record_data, attribute_len);
    attr[attribute_len] = '\0';

    //Check if attribute in selection range.
    if(strcmp(attr, start) >= 0 && strcmp(attr, end) <= 0){
        return 0;
    }
    return 1;
}

void build_path(char* path, char* folder, char* file) {
    strcat(path, folder);
    strcat(path, "/");
    strcat(path, file);
}
