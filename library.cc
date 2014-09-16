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
        memcpy((void*)attr, (char *) buf + i*attribute_len, attribute_len);
        record->push_back(attr);
    }
}

void init_fixed_len_page(Page *page, int page_size, int slot_size) {
    // Initialize fields
    page->page_size = page_size;
    page->slot_size = slot_size;
    page->data = malloc(page_size);

    // Create directory
    
    //Number of slot directories that fit into unslottable space.
    int unslottable_directories = (page_size%slot_size)*8;
    
    //Use record slots for directory if we do not have enough free space
    //for the directory.
    int directories_per_slot = page_size*8;
    int num_directory_slots = 0;
    while(num_directory_slots*directories_per_slot + unslottable_directories < page_size/slot_size - num_directory_slots){
        num_directory_slots++;
    }
    //Calculate the byte offset where the directory starts.
    page->directory_offset = (page_size/slot_size - num_directory_slots)*page->slot_size;
    
    //Set directory to empty.
    memset((unsigned char*)page->data + page->directory_offset, 0, slot_size*num_directory_slots + page_size%slot_size);
    printf("Page Initialized. Page size: %d, Slot size: %d, Slots in page: %d, Directory Bytes: %d, Directory slots: %d\n",
        page_size, slot_size, page_size/slot_size - num_directory_slots, slot_size*num_directory_slots + page_size%slot_size, num_directory_slots);
    
}

int fixed_len_page_capacity(Page *page) {
    return (page->directory_offset)/page->slot_size;
}

int fixed_len_page_freeslots(Page *page) {
    int freeslots = 0;
    
    char* directory = ((char*)page->data) + page->directory_offset;
    
    for(int i = 0; i < fixed_len_page_capacity(page); i++){
        if(i%8 == 0)
            directory++;
        
        if((int)(*directory) >> (i%8) == 0){
            freeslots++;
        }
    }
    return freeslots;
}

int add_fixed_len_page(Page *page, Record *r) {
    unsigned char* directory_offset = ((unsigned char*)page->data) + page->directory_offset;

    //Iterate slots directory to find a free one.
    for(int i = 0; i < fixed_len_page_capacity(page); i++){
        if(i > 0 && i%8 == 0){
            directory_offset+=1;
        }
        unsigned char directory = (unsigned char)*directory_offset;

        if(directory >> (i%8) == 0){
            //Write record to page.
            fixed_len_write(r, ((char*)page->data) + i*page->slot_size);
            
            //Update directory.
            directory |= 1 << (i%8);
            memcpy(directory_offset, &directory, 1);  
            return i;
        }
    }
    
    //Reached here means we didn't find any free slots.
    return -1;
}

void write_fixed_len_page(Page *page, int slot, Record *r) {
    // Pointer to directory slot is found.
    // Pointer to slot is found
    // Use fixed_len_write to write it.
    // Mark header as 1
}

void read_fixed_len_page(Page *page, int slot, Record *r) {
    char* slot_ptr = (char *)page->data + (page->slot_size * slot);
    fixed_len_read(slot_ptr, page->slot_size, r);
}