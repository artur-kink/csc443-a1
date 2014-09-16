#include "library.h"

int fixed_len_sizeof(Record *record){
    return record_size;
}

void fixed_len_write(Record *record, void *buf) {
	for (int i = 0; i < record->size(); i++) {
		memcopy(buf[i*attribute_len], record->at(i), attribute_len);
	}
}