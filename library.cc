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

    // Create directory and set directory to empty.
    memset((unsigned char*)page->data + fixed_len_page_directory_offset(page), 0, page->page_size - fixed_len_page_directory_offset(page));
}

void free_fixed_len_page(Page* page){
    //Free data buffer.
    free(page->data);
    page->data = 0;
    page->page_size = 0;
}

int fixed_len_page_capacity(Page *page) {
    return (fixed_len_page_directory_offset(page))/page->slot_size;
}

int fixed_len_page_directory_offset(Page *page) {
    // Calculate the byte offset where the directory starts.
    return page->page_size - ceil((floor((float)page->page_size/(float)page->slot_size))/8);
}

int fixed_len_page_freeslots(Page *page) {
    int freeslots = 0;

    //Get directory.
    char* directory = ((char*)page->data) +fixed_len_page_directory_offset(page);

    //Loop over directory to see which records are free.
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
    unsigned char* directory_offset = ((unsigned char*)page->data) +fixed_len_page_directory_offset(page);

    //Iterate slots directory to find a free one.
    for(int i = 0; i < fixed_len_page_capacity(page); i++){
        if(i > 0 && i%8 == 0){
            directory_offset += 1;
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
    //Check that slot is in valid space.
    if(slot >= fixed_len_page_capacity(page))
        return;

    //Get byte position of slot in the directory.
    unsigned char* directory_offset = ((unsigned char*)page->data) + fixed_len_page_directory_offset(page);
    directory_offset += slot/8;

    //Update directory, set as written.
    unsigned char directory = (unsigned char)*directory_offset;
    directory |= 1 << (slot%8);
    memcpy(directory_offset, &directory, 1);

    //Write record to slot.
    unsigned char* slot_ptr = ((unsigned char*)page->data) + page->slot_size*slot;
    fixed_len_write(r, slot_ptr);
}

void read_fixed_len_page(Page *page, int slot, Record *r) {
    //Check that slot is in valid space.
    if(slot >= fixed_len_page_capacity(page))
        return;

    //It is up to the caller to make sure the requested slot is actually not empty.
    char* slot_ptr = (char*)page->data + (page->slot_size * slot);
    fixed_len_read(slot_ptr, page->slot_size, r);
}

void init_heapfile(Heapfile *heapfile, int page_size, FILE *file) {
    // number of directory slots is given by the structure defined in the data layout doc
    // We subtract the pointer to another directory file and then divy up the rest of the space by using 1 int to
    // define the amount offree space and another to give the id of the page.
    int number_of_slots_in_heap = floor((page_size - sizeof(int)) / (sizeof(int) + sizeof(int)));

    // We can use 0 to point to no other directory? Otherwise we can use a hid (pid but for heap directories?)
    int next_directory_heap_file_id = 0;
    fwrite(&next_directory_heap_file_id, sizeof(int), 1, file);

    // seek to the spot we shoud start the first iteration of the for loop at.
    // Actually this seem redundant here?
    // fseek(heapfile->file_ptr, sizeof(int), SEEK_SET);
    for (int i = 0; i < number_of_slots_in_heap; i++) {
        // Write the pid
        fwrite(&i, sizeof(int), 1, file);
        // Write the amount of free space on each of the pages, initially the size of the page.
        fwrite(&page_size, sizeof(int), 1, file);
    }

    // I assume we want to do this? But maybe I'm just used to writing files in python.
    rewind(file);
    heapfile->page_size = page_size;
    heapfile->file_ptr = file;
}

PageID alloc_page(Heapfile *heapfile) {
    Page* new_page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(new_page, heapfile->page_size, record_size);

    // See if there's a next directory page
    rewind(heapfile->file_ptr);
    int next_directory_heap_file_id = 0;
    fread(&next_directory_heap_file_id, sizeof(int), 1, heapfile->file_ptr);

    // Lets find the free page.
    int number_of_slots_in_heap = floor((page_size - sizeof(int)) / (sizeof(int) + sizeof(int)));
    PageID page_id = 0;
    // Predefine variables for looping.
    int free_space = 0;
    int current_page_id = 0;
    int current_heap_id = 0;

    // seek to the spot we shoud start the first iteration of the while loop at.
    // This seems redundant.
    // fseek(heapfile->file_ptr, sizeof(int), SEEK_SET);

    // The infamous while loop begins
    while (current_page_id < number_of_slots_in_heap*(current_heap_id + 1)) {
        // Read in current page info for that slot
        fread(&page_id, sizeof(int), 1, heapfile->file_ptr);
        fread(&free_space, sizeof(int), 1, heapfile->file_ptr);
        // Look for enough room for a free page!
        if (free_space == heapfile->page_size) {
            // We found it, lets bail and return the page_id;
            rewind(heapfile->file_ptr);
            return page_id;
        }
        // Lets look some more.
        current_page_id++;

        // If we reach the end of the pages in this directoruy
        if (current_page_id == number_of_slots_in_heap*(current_heap_id + 1)) {
            // We'll name this heapId
            current_heap_id++;

            if (next_directory_heap_file_id > 0) {
                // seek to the correct spot. Which is (page_size)*(slots + directory)*(current_heap_id)
                // Number of directories we are seeking is correct since we incriemtned and use an initial value of 0 so
                // the first time it will be 1....then 2.....
                fseek(heapfile->file_ptr, heapfile->page_size*(number_of_slots_in_heap + 1)*(current_heap_id), SEEK_SET);
                // Store the next id
                fread(&next_directory_heap_file_id, sizeof(int), 1, heapfile->file_ptr);
                // back to the while loop.
            } else {
                // We must create a new directory page and write the id of the new one to the old one.

                // Seek to the last pages heap_id to set it
                fseek(heapfile->file_ptr, heapfile->page_size*(number_of_slots_in_heap + 1)*(current_heap_id - 1), SEEK_SET);
                fwrite(&current_heap_id, sizeof(int), 1, heapfile->file_ptr);

                // Seek to new directory page
                fseek(heapfile->file_ptr, heapfile->page_size*(number_of_slots_in_heap + 1)*(current_heap_id), SEEK_SET);
                // write there is no next heap file to this one
                int next_directory_heap_file_id = 0;
                fwrite(&next_directory_heap_file_id, sizeof(int), 1, heapfile->file_ptr);

                // Intialize the directory page
                for (int i = current_page_id; i < number_of_slots_in_heap*(current_heap_id + 1); i++) {
                    // Write the pid
                    fwrite(&i, sizeof(int), 1, heapfile->file_ptr);
                    // Write the amount of free space on each of the pages, initially the size of the page.
                    fwrite(&heapfile->page_size, sizeof(int), 1, heapfile->file_ptr);
                }
            }
            // seek to the spot we shoud start the next iteration of the while loop at.
            fseek(heapfile->file_ptr, heapfile->page_size*(number_of_slots_in_heap + 1)*(current_heap_id), SEEK_SET);
        }
        // We dun goofed
        return -1;
    }

    // JOSEPH: Did not understand what this was doing. But looking at here page http://dblab.cs.toronto.edu/courses/443/2014/03.data-layout.html section 1.2...the above makes more sense to me.

    //Find end of heapfile.
    // fseek(heapfile->file_ptr, 0, SEEK_END);
    // fwrite(new_page->data, new_page->page_size, 1, heapfile->file_ptr);
    // fflush(heapfile->file_ptr);
    // free_fixed_len_page(new_page);
    // free(new_page);

    //Calculate the id of new allocated page.
    // fpos_t cur_pos;
    // fgetpos(heapfile->file_ptr, &cur_pos);
    // return cur_pos.__pos/heapfile->page_size - 1;
}

void read_page(Heapfile *heapfile, PageID pid, Page *page) {
    // Why would read initialize a page?
    // init_fixed_len_page(page, heapfile->page_size, record_size);

    int heap_id_of_page = floor(pid/((heapfile->page_size - sizeof(int)) / (sizeof(int) + sizeof(int))));

    // Seek to the correct spots (number of directories + number of pid's) * (page_size)
    fseek(heapfile->file_ptr, heapfile->page_size * ((heap_id_of_page + 1) + pid), SEEK_SET);
    fread(page->data, page->page_size, 1, heapfile->file_ptr);
    rewind(heapfile->file_ptr);
}

void write_page(Page *page, Heapfile *heapfile, PageID pid) {
    // look above.
    int heap_id_of_page = floor(pid/(heapfile->page_size - sizeof(int)) / (sizeof(int) + sizeof(int)));
    fseek(heapfile->file_ptr, heapfile->page_size * ((heap_id_of_page + 1) + pid), SEEK_SET);
    fwrite(page->data, page->page_size, 1, heapfile->file_ptr);

    // We cant remove a page so we know the freespace. we now have to write that in the directory.
    int new_number_of_pages_written_in_this_directory = (pid % ((heapfile->page_size - sizeof(int)) / (sizeof(int) + sizeof(int))));
    int location_of_new_page = sizeof(int) + (new_number_of_pages_written_in_this_directory)*(sizeof(int) + sizeof(int));
    fseek(location_of_new_page + sizeof(int));

    int free_space_in_page = 0;
    fwrite(&free_space_in_page, sizeof(int), 1, heapfile->file_ptr);


    rewind(heapfile->file_ptr);
    fflush(heapfile->file_ptr);
}

RecordIterator::RecordIterator(Heapfile *heapfile) {
    this->heap = heapfile;
    fseek(heapfile->file_ptr, 0, SEEK_SET);
    
    //Start at first page.
    this->current_page_id = 0;
    this->current_page = (Page*)malloc(sizeof(Page));
    this->current_slot = 0;

    read_page(heap, current_page_id, current_page);
}

Record RecordIterator::next() {
    Record record;
    read_fixed_len_page(this->current_page, this->current_slot, &record);

    // go to the next slot for this page, so we don't continuously read
    // the same record over and over again
    this->current_slot++;

    return record;
}

bool RecordIterator::hasNext() {
    while (!feof(this->heap->file_ptr)) {

        unsigned char *directory_offset = ((unsigned char *) this->current_page->data) + fixed_len_page_directory_offset(this->current_page) + ((current_slot-1)/8);

        //Find next available record.
        for (; this->current_slot < fixed_len_page_capacity(this->current_page); this->current_slot++) {
            if (this->current_slot > 0 && this->current_slot % 8 == 0) {
                directory_offset += 1;
            }

            //Retrieve record from slot if its marked in the directory.
            unsigned char directory = *directory_offset;
            if (directory >> (this->current_slot % 8) & 0x01) {
                return true;
            }
        }

        this->current_page_id++;
        this->current_slot = 0;
        free_fixed_len_page(this->current_page);
        read_page(this->heap, this->current_page_id, this->current_page);
    }

    return false;
}

RecordIterator::~RecordIterator(){
    //Free the current page memory.
    free_fixed_len_page(current_page);
    free(current_page);
    
    heap = 0;
    current_page = 0;
}