#include "library.h"

int fixed_len_sizeof(Record *record){
    return record_size;
}

void fixed_len_write(Record *record, void *buf) {
	for (int i = 0; i < num_attributes; i++) {
		memcpy((V)buf + i*attribute_len, record->at(i), attribute_len);
	}
}

void fixed_len_read(void *buf, int size, Record *record) {
	for (int i = 0; i < num_attributes; i++) {
		// V* thing = new V[attribute_len];
		// memcpy(thing, buf + i*attribute_len, attribute_len);
	}
}
