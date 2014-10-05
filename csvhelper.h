#include <stdio.h>
#include <string.h>
#include <vector>

#include "library.h"

/**
 * Parse the slot number and pid of a record id string.
 */
int parse_record_id(const char* id, int* pid);

/**
 * Print given record as a csv.
 */
void print_record(Record* record);

/**
 * Read records from given file name into a Record vector.
 */
int read_records(const char* file, std::vector<Record*>* records);