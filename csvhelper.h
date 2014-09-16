#include <stdio.h>
#include <vector>

#include "library.h"

/**
 * Print given record as a csv.
 */
void print_record(Record* record);

/**
 * Read records from given file name into a Record vector.
 */
void read_records(const char* file, std::vector<Record*>* records);