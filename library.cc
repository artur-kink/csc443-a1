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
        // What about initializing to 0, do we need to do that?
        // memset(attr, 0, attribute_len);
        memcpy((void*)attr, (char *) buf + i*attribute_len, attribute_len);
        record->push_back(attr);
    }
}

void init_fixed_len_page(Page *page, int page_size, int slot_size) {
    // Initialize fields
    page->page_size = page_size;
    page->slot_size = slot_size;
    // zero initializing the array as calloc does, yay I'm learning something C, seems relevant to out interests.
    page->data = calloc(page_size, sizeof(char));

    // Directory
    // Find where header is and use the 16 bits (or however many records we are storing) so we can use them as a bitmap
}

int fixed_len_page_capacity(Page *page) {
    // Unsure if correct, depends how pages work including the directory....but the simple and wrong answer would be:
    return floor((page->page_size)/(page->slot_size));
}

int fixed_len_page_freeslots(Page *page) {
    // Same issue as init.
    // 1 .Navigate to end of Directory
    // 2. Loop over fixed_len_page_capcity number of times
    // 3. count those in the loop that are set as 0
    // 4 return that number
    return 0;
}

int add_fixed_len_page(Page *page, Record *r) {
    // Same as fixed_len_page_freeslots
    // 1. go to header
    if (fixed_len_page_freeslots(page) == 0) {
        return -1;
    }
    // 2. loop over header till 0 is found.
    // 3. calculate spot to naviagte to based on index in directory
    // 4. go to record slot
    // 5. fixed_len_write in that slot
    // 6. return 0
    // 7. if we somehow looped over them all and found no slot, return -1
    return -1;
}


