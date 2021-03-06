#include "library.h"

int fixed_len_sizeof(Record *record){
    return record_size;
}

void fixed_len_write(Record *record, void *buf) {
    for (int i = 0; i < record->size(); i++) {
        memcpy(((char*)buf + i*attribute_len), record->at(i), attribute_len);
    }
}

void fixed_len_read(void *buf, int size, Record *record) {
    for (int i = 0; i < size/attribute_len; i++) {
        V attr = (char *) buf + i*attribute_len;
        record->push_back(attr);
    }
}

void init_fixed_len_page(Page *page, int page_size, int slot_size) {
    // Initialize fields
    page->page_size = page_size;
    page->slot_size = slot_size;
    page->data = malloc(page_size);

    // Create directory and set directory to empty.
    memset(get_directory(page), 0, page->page_size - fixed_len_page_directory_offset(page));
}

unsigned char* get_directory(Page* page){
    return (unsigned char*)page->data + fixed_len_page_directory_offset(page);
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

bool is_freeslot(Page* page, int slot){
    unsigned char directory = *(get_directory(page) + slot / 8);
    return (directory & (1 << (slot % 8))) == 0;
}

int fixed_len_page_directory_offset(Page *page) {
    // Calculate the byte offset where the directory starts.
    return page->page_size - ceil((floor((float)page->page_size/(float)page->slot_size))/8);
}

std::vector<int> fixed_len_page_freeslot_indices(Page *page) {
    std::vector<int> freeslots;

    //Get directory.
    unsigned char* directory_offset = get_directory(page);

    //Loop over directory to see which records are free.
    for(int i = 0; i < fixed_len_page_capacity(page); i++) {
        if(i > 0 && i%8 == 0)
            directory_offset++;

        unsigned char directory = *directory_offset;

        if((directory & (1 << (i%8))) == 0){
            freeslots.push_back(i);
        }
    }

    return freeslots;
}

int fixed_len_page_freeslots(Page* page){
    return fixed_len_page_freeslot_indices(page).size();
}

int add_fixed_len_page(Page *page, Record *r) {
    unsigned char* directory_offset = get_directory(page);

    //Iterate slots directory to find a free one.
    for(int i = 0; i < fixed_len_page_capacity(page); i++){
        if(i > 0 && i%8 == 0)
            directory_offset++;

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
    unsigned char* directory_offset = get_directory(page);
    directory_offset += slot/8;

    //Update directory, set as written.
    unsigned char directory = *directory_offset;
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

int number_of_pages_in_heap_directory(int page_size) {
    // number of directory slots is given by the structure defined in the data layout doc
    // We subtract the pointer to another directory file and then divvy up the rest of the space by using 1 int to
    // define the amount of free space and another to give the id of the page.
    return floor((page_size - sizeof(int)) / (2 * sizeof(int)));
}

int offset_to_directory(int directory_id, int page_size) {
    // Assuming starting with 0
    // It's (the number of slots + 1 (for the directory)) multiplied by the directory we want.
    return page_size * (number_of_pages_in_heap_directory(page_size) + 1) * (directory_id);
}

PageID last_pid_of_directory(int directory_id, int page_size) {
    return number_of_pages_in_heap_directory(page_size) * (directory_id + 1);
}

int open_heapfile(Heapfile *heap, char *path, int page_size, int slot_size) {
    FILE* heap_file = fopen(path, "rb");
    if(!heap_file){
        fprintf(stderr, "Failed to open heap file: %s\n", path);
        free(heap);
        fclose(heap_file);
        return 2;
    }
    heap->page_size = page_size;
    heap->slot_size = slot_size;
    heap->file_ptr = heap_file;
    return 0;
}

void init_heapfile(Heapfile *heapfile, int page_size, FILE *file) {
    int number_of_pages_in_heap = number_of_pages_in_heap_directory(page_size);

    // A next heap id of 0 indicates that there are no more directory pages
    // after this one.
    int next_directory_heap_file_id = 0;
    fwrite(&next_directory_heap_file_id, sizeof(int), 1, file);

    for (int i = 0; i < number_of_pages_in_heap; i++) {

        // Write the pid
        fwrite(&i, sizeof(int), 1, file);

        // Write the amount of free space on each of the pages, initially the size of the page.
        fwrite(&page_size, sizeof(int), 1, file);
    }

    // I assume we want to rewind the file? But maybe I'm just used to writing files in python.
    rewind(file);
    heapfile->page_size = page_size;
    heapfile->file_ptr = file;
}

PageID alloc_page(Heapfile *heapfile, Page* dir_page, PageID current_page_id) {
//    printf("running with current page id %d\n", current_page_id);
    char* dp_data = (char*)dir_page->data;
    int next_directory_heap_file_id = *(int*)dp_data;

    int page_index = current_page_id % number_of_pages_in_heap_directory(heapfile->page_size);
    dp_data += sizeof(int) + sizeof(int) * 2 * page_index;

    Page* new_page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(new_page, heapfile->page_size, heapfile->slot_size);

    PageID current_heap_id = heap_id_of_page(current_page_id, heapfile->page_size);
//    printf("current heap id %d\n", current_heap_id);

    // Predefine variables for looping.
    int free_space = 0;
    PageID last_heap_pid = last_pid_of_directory(current_heap_id, heapfile->page_size);
//    printf("last heap pid %d\n", last_heap_pid);

    while (true) {

        // Read in freespace for that slot
        dp_data += sizeof(int);
        free_space = *(int*)dp_data;

//        printf("current_page_id %d, free space: %d\n", current_page_id, free_space);
//        printf("examining pid %d\n", current_page_id);

        // Check if we have enough room for a free page!

        if (free_space == heapfile->page_size) {
            // set the freespace to 0 at this point in the directory page
            *((int*)dp_data) = 0;

            return current_page_id;
        }

        // We don't have room for a free page, so let's look some more.
        dp_data += sizeof(int);
        current_page_id++;

        if (current_page_id >= last_heap_pid) {

            current_heap_id++;
            last_heap_pid = last_pid_of_directory(current_heap_id, heapfile->page_size);

            if (next_directory_heap_file_id > 0) {

                // seek to the correct spot.
                // Number of directories we are seeking is correct since we incremented
                read_directory_page(heapfile, current_heap_id, dir_page);
                dp_data = (char*)dir_page->data;

                // Store the next id
                next_directory_heap_file_id = (int)*dp_data;
                dp_data += sizeof(int);
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
                for (int i = current_page_id; i < last_heap_pid; i++) {

                    // Write the pid
                    fwrite(&i, sizeof(int), 1, heapfile->file_ptr);

                    // Write the amount of free space on each of the pages,
                    // initially the size of the page.
                    fwrite(&heapfile->page_size, sizeof(int), 1, heapfile->file_ptr);
                }

                read_directory_page(heapfile, current_heap_id, dir_page);
                dp_data = (char*)dir_page->data + sizeof(int);
            }
        }
    }

    return -1;
}

PageID alloc_page(Heapfile *heapfile) {
    Page* new_page = (Page*)malloc(sizeof(Page));
    init_fixed_len_page(new_page, heapfile->page_size, heapfile->slot_size);

    // See if there's a next directory page
    rewind(heapfile->file_ptr);
    int next_directory_heap_file_id = 0;
    fread(&next_directory_heap_file_id, sizeof(int), 1, heapfile->file_ptr);

    // Lets find the free page.
    int number_of_pages_in_heap = number_of_pages_in_heap_directory(heapfile->page_size);
    PageID current_page_id = 0;

    // Predefine variables for looping.
    int free_space = 0;
    int current_heap_id = 0;
    PageID last_heap_pid = last_pid_of_directory(current_heap_id, heapfile->page_size);

    while (current_page_id < last_heap_pid) {

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
        if (current_page_id == last_heap_pid) {

            current_heap_id++;
            last_heap_pid = last_pid_of_directory(current_heap_id, heapfile->page_size);

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
                for (int i = current_page_id; i < last_heap_pid; i++) {

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

    return -1;
}

bool out_of_bounds(PageID pid, Heapfile* heap) {
    fseek(heap->file_ptr, 0, SEEK_END);
    int file_length = ftell(heap->file_ptr);
    fseek(heap->file_ptr, 0, SEEK_SET);

    return pid < 0 || offset_of_pid(pid, heap->page_size) > file_length;
}

PageID heap_id_of_page(PageID pid, int page_size) {
    return floor(pid / number_of_pages_in_heap_directory(page_size));
}

int offset_of_pid(PageID pid, int page_size) {
    // (number of directories + number of pid's) * (page_size)
    return page_size * ((heap_id_of_page(pid, page_size) + 1) + pid);
}

void read_directory_page(Heapfile *heapfile, PageID directory_id, Page *page) {
    init_fixed_len_page(page, heapfile->page_size, heapfile->slot_size);

    // Seek to the correct spot
    fseek(heapfile->file_ptr, offset_to_directory(directory_id, heapfile->page_size), SEEK_SET);
    fread(page->data, page->page_size, 1, heapfile->file_ptr);

    rewind(heapfile->file_ptr);
}

void write_directory_page(Heapfile *heapfile, PageID directory_id, Page *dir_page) {
    fseek(heapfile->file_ptr, offset_to_directory(directory_id, heapfile->page_size), SEEK_SET);
    fwrite(dir_page->data, dir_page->page_size, 1, heapfile->file_ptr);
}

void read_page(Heapfile *heapfile, PageID pid, Page *page) {
    init_fixed_len_page(page, heapfile->page_size, heapfile->slot_size);

    // Seek to the correct spot
    fseek(heapfile->file_ptr, offset_of_pid(pid, heapfile->page_size), SEEK_SET);
    fread(page->data, page->page_size, 1, heapfile->file_ptr);

    rewind(heapfile->file_ptr);
}

int try_read_page(Heapfile *heapfile, PageID pid, Page *page) {
    if (out_of_bounds(pid, heapfile))
        return -1;

    read_page(heapfile, pid, page);
    return 0;
}

void write_page(Page *page, Heapfile *heapfile, PageID pid) {

    // look above.
    int heap_id = heap_id_of_page(pid, page->page_size);

    fseek(heapfile->file_ptr, offset_of_pid(pid, heapfile->page_size), SEEK_SET);
    fwrite(page->data, page->page_size, 1, heapfile->file_ptr);

    // Seek to the free space bit of this pid.
    int slot_index = pid % number_of_pages_in_heap_directory(page->page_size);
    int offset_of_directory_entry = sizeof(int) + sizeof(int) * 2 * slot_index;
    fseek(heapfile->file_ptr, offset_to_directory(heap_id, heapfile->page_size) + offset_of_directory_entry + sizeof(int), SEEK_SET);

    int free_space_in_page = fixed_len_page_freeslots(page) * page->slot_size;
    fwrite(&free_space_in_page, sizeof(int), 1, heapfile->file_ptr);

    rewind(heapfile->file_ptr);
    fflush(heapfile->file_ptr);
}

PageID seek_page(Page* page, Page* dir_page, PageID start_pid, Heapfile* heap, bool should_be_occupied) {
    if (out_of_bounds(start_pid, heap))
        return -1;

    int page_size = heap->page_size;
    int slots_in_heap = number_of_pages_in_heap_directory(page_size);

    PageID current_pid = start_pid;
    PageID heap_id = heap_id_of_page(start_pid, page_size);

    // next heap id is initialized when we read the first directory page
    PageID next_heap_id = -1;

    do {
        PageID last_page_id = last_pid_of_directory(heap_id, page_size);

        if ((current_pid == last_pid_of_directory(heap_id - 1, page_size)) || current_pid == 0) {
            read_directory_page(heap, heap_id, dir_page);
        }

        char* dp_data = (char*)(dir_page->data);
        next_heap_id = *(int*)(dp_data);

        for (; current_pid < last_page_id; current_pid++) {
            int page_index = current_pid % slots_in_heap;
            int page_offset = sizeof(int) + page_index * sizeof(int)*2;
            int freespace = *(int*) (dp_data + page_offset + sizeof(int));
            int pid = *(int*) (dp_data + page_offset);

            if ((should_be_occupied && freespace < page_size) || (!should_be_occupied && freespace >= page->slot_size)) {
                read_page(heap, current_pid, page);
                return current_pid;
            }
        }

        heap_id++;
    } while (next_heap_id > 0);

    return -1;
}

RecordIterator::RecordIterator(Heapfile *heapfile) {
    this->heap = heapfile;
    rewind(heapfile->file_ptr);

    //Start at first page.
    this->current_page_id = 0;
    this->current_page = (Page*)malloc(sizeof(Page));
    this->current_directory_page = (Page*)malloc(sizeof(Page));
    this->current_slot = 0;

    init_fixed_len_page(this->current_page, heapfile->page_size, heapfile->slot_size);
    init_fixed_len_page(this->current_directory_page, heapfile->page_size, heapfile->slot_size);

    this->current_page_id = seek_page(this->current_page, this->current_directory_page, this->current_page_id, this->heap, true);
}

Record RecordIterator::next() {
    Record record;
    read_fixed_len_page(this->current_page, this->current_slot, &record);

    this->current_slot++;

    return record;
}

void RecordIterator::printRecords(Record *record) {
    //Iterate all records and print the 10 bytes as characters.
    for(int i = 0; i < record->size()-1; i++){
        printf("Record %d%.5d: %.10s,\n", this->current_page_id, i, record->at(i));
    }
    //Print the last variable with no trailing comma.
    printf("Record %d%.5ld: %.10s\n", this->current_page_id, record->size() - 1, record->at(record->size()-1));

}

bool RecordIterator::hasNext() {
    while (true) {
        while (this->current_slot < fixed_len_page_capacity(this->current_page)) {
            unsigned char* dir_slot_offset = get_directory(this->current_page);
            for (int k = 0; k < floor(this->current_slot / 8); k++)
                dir_slot_offset++;
            unsigned char directory_byte_for_slot = *dir_slot_offset;
            if ((directory_byte_for_slot >> (this->current_slot % 8) != 0)) {
                break;
            }
            this->current_slot++;
        }
        // If we are above the slot capacity, we read in the next page.
        if (this->current_slot == fixed_len_page_capacity(this->current_page)) {
            this->current_slot = 0;
            this->current_page_id++;

            this->current_page_id = seek_page(this->current_page, this->current_directory_page, this->current_page_id, this->heap, true);
        } else {
            break;
        }
    }

    return this->current_page_id != -1;
}

RecordIterator::~RecordIterator(){
    //Free the current page memory.
    free_fixed_len_page(current_page);
    free(current_page);

    heap = 0;
    current_page = 0;
}
