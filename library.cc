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
    
    int directories_per_slot = page_size*8;
    int num_directory_slots = 0;
    while(num_directory_slots*directories_per_slot + unslottable_directories < page_size/slot_size - num_directory_slots){
        num_directory_slots++;
    }
    page->directory_slots = num_directory_slots;
    
    //Set directory to empty.
    memset((unsigned char*)page->data + fixed_len_page_capacity(page)*page->slot_size, 0, slot_size*num_directory_slots + page_size%slot_size);
    printf("Page Initialized. Page size: %d, Slot size: %d, Slots in page: %d, Directory Bytes: %d, Directory slots: %d\n", page_size, slot_size, page_size/slot_size - num_directory_slots, slot_size*num_directory_slots + page_size%slot_size, num_directory_slots);
    
}

int fixed_len_page_capacity(Page *page) {
    return page->page_size/page->slot_size - page->directory_slots;
}

int fixed_len_page_freeslots(Page *page) {
    int freeslots = 0;
    int page_capacity = fixed_len_page_capacity(page);
    
    char* directory = ((char*)page->data) + page_capacity*page->slot_size;
    
    for(int i = 0; i < page_capacity; i++){
        if(i%8 == 0)
            directory++;
        
        if((int)(*directory) >> (i%8) == 0){
            freeslots++;
        }
    }
    return freeslots;
}

int add_fixed_len_page(Page *page, Record *r) {
    int page_capacity = fixed_len_page_capacity(page);
    unsigned char* directory_offset = ((unsigned char*)page->data) + page_capacity*page->slot_size;

    //Iterate slots directory to find a free one.
    for(int i = 0; i < page_capacity; i++){
        if(i > 0 && i%8 == 0){
            directory_offset+=1;
        }
        unsigned char directory = *directory_offset;

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

void init_heapfile(Heapfile *heapfile, int page_size, FILE *file) {
    heapfile->page_size = page_size;
    heapfile->file_ptr = file;

    heapfile->num_pages = 0;
    heapfile->directory = NULL;

    // TODO: How does the directory map back to actual pages?
}


PageID alloc_page(Heapfile *heapfile) {
    Page* new_page = new Page;
    init_fixed_len_page(new_page, heapfile->page_size, sizeof(int));

    PageEntry* new_entry = new PageEntry(
            heapfile->num_pages * heapfile->page_size,
            fixed_len_page_capacity(new_page),
            NULL
    );

    // add page info to directory
    if (heapfile->num_pages == 0) {
        heapfile->directory = new_entry;
    } else {
        PageEntry* cur_dir = heapfile->directory;
        while ((cur_dir->next) != NULL)
            cur_dir = cur_dir->next;

        cur_dir->next = new_entry;
    }

    // TODO: What should we write to the file at this point?

    return heapfile->num_pages++;
}

void read_page(Heapfile *heapfile, PageID pid, Page *page) {
//    for (int i = 0; i < pid; i++) {
//        fseek(heapfile->file_ptr, , SEEK_CUR);
//    }
}

void write_page(Page *page, Heapfile *heapfile, PageID pid) {
    int num_pages_to_seek = alloc_page(heapfile);


}
