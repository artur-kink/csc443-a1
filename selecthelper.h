#include <stdio.h>
#include <string.h>
#include <vector>

#include "library.h"

/**
 * Return 0 current record is between start and end, else 1. Reads record into attr.
 */
int compare_record(char* attr, const char* record_data, char* start, char* end);

/**
 * Build path and put into `path` given folder name a filename.
 */
void build_path(char* path, char* folder, char* file);
