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

std::vector<int> fixed_len_page_freeslots(Page *page) {
    std::vector<int> freeslots;

    //Get directory.
    char* directory = ((char*)page->data) + fixed_len_page_directory_offset(page);

    //Loop over directory to see which records are free.
    for(int i = 0; i < fixed_len_page_capacity(page); i++){
        if(i%8 == 0)
            directory++;

        if((int)(*directory) >> (i%8) == 0){
            freeslots.push_back(i);
        }
    }

    return freeslots;
}

int add_fixed_len_page(Page *page, Record *r) {
    unsigned char* directory_offset = ((unsigned char*)page->data) + fixed_len_page_directory_offset(page);

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

int number_of_slots_in_heap_directory(int page_size) {
    // number of directory slots is given by the structure defined in the data layout doc
    // We subtract the pointer to another directory file and then divvy up the rest of the space by using 1 int to
    // define the amount of free space and another to give the id of the page.
    return floor((page_size - sizeof(int)) / (2 * sizeof(int)));
}

int offset_to_directory(int directory_id, int page_size) {
    // Assuming starting with 0
    // It's (the number of slots + 1 (for the directory)) multiplied by the directory we want.
    return page_size * (number_of_slots_in_heap_directory(page_size) + 1) * (directory_id);
}

int directory_size(int page_size) {
    return offset_to_directory(1, page_size);
}

void init_heapfile(Heapfile *heapfile, int page_size, FILE *file) {
    int number_of_slots_in_heap = number_of_slots_in_heap_directory(page_size);

    // We can use 0 to point to no other directory? Otherwise we can use a hid (pid but for heap directories?)
    int next_directory_heap_file_id = 0;
    fwrite(&next_directory_heap_file_id, sizeof(int), 1, file);

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
    int number_of_slots_in_heap = number_of_slots_in_heap_directory(heapfile->page_size);
    PageID current_page_id = 0;

    // Predefine variables for looping.
    int free_space = 0;
    int current_heap_id = 0;

    while (current_page_id < number_of_slots_in_heap * (current_heap_id + 1)) {

        // Read in current page info for that slot
        fread(&current_page_id, sizeof(int), 1, heapfile->file_ptr);
        fread(&free_space, sizeof(int), 1, heapfile->file_ptr);

        // Check if we have enough room for a free page!
        if (free_space == heapfile->page_size) {

            // We found it, lets bail and return the current_page_id;
            rewind(heapfile->file_ptr);
            return current_page_id;
        }

        // We don't have room for a free page, so let's look some more.
        current_page_id++;

        // If we reach the end of the pages in this directory
        if (current_page_id == number_of_slots_in_heap*(current_heap_id + 1)) {

            current_heap_id++;

            if (next_directory_heap_file_id > 0) {

                // seek to the correct spot. Which is (page_size)*(slots + directory)*(current_heap_id)
                // Number of directories we are seeking is correct since we incremented and use an initial value of 0 so
                // the first time it will be 1....then 2.....
                fseek(heapfile->file_ptr, offset_to_directory(current_heap_id, heapfile->page_size), SEEK_SET);

                // Store the next id
                fread(&next_directory_heap_file_id, sizeof(int), 1, heapfile->file_ptr);
            } else {
                // We must create a new directory page and
                // write the id of the new one to the old one.

                // Seek to the last pages heap_id to set it
                fseek(heapfile->file_ptr, offset_to_directory(current_heap_id - 1, heapfile->page_size), SEEK_SET);
                fwrite(&current_heap_id, sizeof(int), 1, heapfile->file_ptr);

                // Seek to new directory page
                fseek(heapfile->file_ptr, offset_to_directory(current_heap_id, heapfile->page_size), SEEK_SET);

                // write there is no next heap file to this one
                int next_directory_heap_file_id = 0;
                fwrite(&next_directory_heap_file_id, sizeof(int), 1, heapfile->file_ptr);

                // Initialize the directory page
                for (int i = current_page_id; i < number_of_slots_in_heap * (current_heap_id + 1); i++) {

                    // Write the pid
                    fwrite(&i, sizeof(int), 1, heapfile->file_ptr);

                    // Write the amount of free space on each of the pages,
                    // initially the size of the page.
                    fwrite(&heapfile->page_size, sizeof(int), 1, heapfile->file_ptr);
                }
            }

            // seek to the spot we should start the next iteration of the while loop at.
            fseek(heapfile->file_ptr, offset_to_directory(current_heap_id, heapfile->page_size) + sizeof(int), SEEK_SET);
        }
    }

    // We dun goofed
    return -1;
}

bool is_directory_pid(PageID pid, int page_size) {
    return pid % number_of_slots_in_heap_directory(page_size) == 0;
}

int offset_of_pid(PageID pid, int page_size) {
    int heap_id_of_page = floor(pid / number_of_slots_in_heap_directory(page_size));

    // We cant remove a page so we know the freespace. we now have to write that in the directory.
    // (number of directories + number of pid's) * (page_size)
    return page_size * ((heap_id_of_page + 1) + pid);
}

void read_page(Heapfile *heapfile, PageID pid, Page *page) {
    init_fixed_len_page(page, heapfile->page_size, record_size);

    // Seek to the correct spot
    fseek(heapfile->file_ptr, offset_of_pid(pid, heapfile->page_size), SEEK_SET);
    fread(page->data, page->page_size, 1, heapfile->file_ptr);

    rewind(heapfile->file_ptr);
}

void write_page(Page *page, Heapfile *heapfile, PageID pid) {

    // look above.
    int heap_id_of_page = floor(pid / number_of_slots_in_heap_directory(heapfile->page_size));

    fseek(heapfile->file_ptr, offset_of_pid(pid, heapfile->page_size), SEEK_SET);
    fwrite(page->data, page->page_size, 1, heapfile->file_ptr);

    // Seek to the free space bit of this pid.
    int offset_of_directory = offset_to_directory(heap_id_of_page, heapfile->page_size);
    int offset_of_directory_entry = offset_of_directory + sizeof(int) + (sizeof(int) * 2) * (pid % number_of_slots_in_heap_directory(heapfile->page_size));
    fseek(heapfile->file_ptr, offset_of_directory_entry + sizeof(int), SEEK_SET);

    int free_space_in_page = 0;
    fwrite(&free_space_in_page, sizeof(int), 1, heapfile->file_ptr);

    rewind(heapfile->file_ptr);
    fflush(heapfile->file_ptr);
}

PageID seek_page(Page* page, Page* dir_page, int start_pid, Heapfile* heap, bool should_be_occupied) {
    /*
     1. figure out the current directory pid from the start_pid -- simple arithmetic
        since we know how many data pages each directory "points" to
     2. subtract that current_dir_pid from our start pid to get the index of the slot
        that tells us how much our current page has
     3. read dir page at dir pid (I guess we wouldn't need to do this on the
                                  first iteration, or on many iterations at all,
                                  since the current dir page will already be
                                  in dir_page)
     4. seek to that slot in dir page
     5. if should_be_occupied and the amount of freespace listed in the slot is 0
        OR not should_be_occupied and freespace > 0:
            read the page with the current pid from heap
            return page id at the current page slot
     6. else, repeat with current pid incremented.
     7. return -1 if our current pid doesn't exist in the file
     */

    return -1;
}

RecordIterator::RecordIterator(Heapfile *heapfile) {
    this->heap = heapfile;
    rewind(heapfile->file_ptr);
    
    //Start at first page.
    this->current_page_id = 0;
    this->current_directory_page_id = 0;
    this->current_page = (Page*)malloc(sizeof(Page));
    this->current_directory_page = (Page*)malloc(sizeof(Page));
    this->current_slot = 0;

    init_fixed_len_page(this->current_page, heapfile->page_size, record_size);
    init_fixed_len_page(this->current_directory_page, heapfile->page_size, record_size);

    read_page(this->heap, this->current_page_id, this->current_page);
}

Record RecordIterator::next() {
    Record record;
    read_fixed_len_page(this->current_page, this->current_slot, &record);

    this->current_slot++;

    return record;
}

bool RecordIterator::hasNext() {
    // If we are above the slot capacity, we read in the next page.
    if (this->current_slot == fixed_len_page_capacity(this->current_page)) {
        this->current_slot = 0;
        this->current_page_id++;

        this->current_page_id = seek_page(this->current_page, this->current_directory_page, this->current_page_id, this->heap, true);
    }

    // If there is something in the page's directory, then we
    // know that the page must exist since we don't have a way to delete records.
//    char* offset_direct_of_next_slot = ((char*)this->current_page->data) + fixed_len_page_directory_offset(this->current_page) + (char)floor(this->current_slot / 8);
//    int directory_bit_for_slot = (int)(*offset_direct_of_next_slot) >> ((this->current_slot) % 8);
//    return (directory_bit_for_slot != 0);

    // TODO: is this sufficient? current_page_id becomes -1 when seeking a page with some
    // data doesn't work out.
    return this->current_page_id > -1;
}

RecordIterator::~RecordIterator(){
    //Free the current page memory.
    free_fixed_len_page(current_page);
    free(current_page);
    
    heap = 0;
    current_page = 0;
}
