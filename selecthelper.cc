#include <string.h>
#include <sys/timeb.h>

#include "library.h"

int compare_record(const char* record_data, char* start, char* end){
    //Check if attribute in selection range.
    if(strncmp(record_data, start, 5) >= 0 && strncmp(record_data, end, 5) <= 0){
        return 0;
    }
    return 1;
}

void build_path(char* path, char* folder, char* file) {
    strcat(path, folder);
    strcat(path, "/");
    strcat(path, file);
}
