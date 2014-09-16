#include "library.h"

int fixed_len_sizeof(Record *record){
    return record_size;
}

void fixed_len_write(Record *record, void *buf) {
    for (int i = 0; i < num_attributes; i++) {
        memcpy(((char*)buf + i*attribute_len), record->at(i), attribute_len);
    }
}

void fixed_len_read(void *buf, int size, Record *record) {
    for (int i = 0; i < num_attributes; i++) {
        V attr = (V)malloc(attribute_len);
        memcpy((void*)attr, buf + i*attribute_len, attribute_len);
        record->push_back(attr);
    }
}

void init_fixed_len_page(Page *page, int page_size, int slot_size){
    page->page_size = page_size;
    page->slot_size = slot_size;
    page->data = malloc(page_size);
}

int fixed_len_page_capacity(Page *page){
    return page->page_size/page->slot_size;
}

int add_fixed_len_page(Page *page, Record *r){
    return 0;
}