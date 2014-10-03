# Makefile
CC = g++
RM = rm

all: write_fixed_len_pages read_fixed_len_page csv2heapfile scan select insert update

library.o: library.cc library.h
	$(CC) -o $@ -c $<

library: library.o

csvhelper.o: csvhelper.cc csvhelper.h
	$(CC) -o $@ -c $<

csvhelper: csvhelper.o

write_fixed_len_pages: write_fixed_len_pages.cc library.o csvhelper.o
	$(CC) -o $@ $< library.o csvhelper.o
	
read_fixed_len_page: read_fixed_len_page.cc library.o csvhelper.o
	$(CC) -o $@ $< library.o csvhelper.o

csv2heapfile: csv2heapfile.cc csvhelper.o library.o
	$(CC) -o $@ $< library.o csvhelper.o

scan: scan.cc library.o csvhelper.o
	$(CC) -o $@ $< library.o csvhelper.o

insert: insert.cc library.o csvhelper.o
	$(CC) -o $@ $< library.o csvhelper.o

update: update.cc library.o csvhelper.o
	$(CC) -o $@ $< library.o csvhelper.o

delete: delete.cc library.o
	$(CC) -o $@ $< library.o

select: select.cc library.o csvhelper.o
	$(CC) -o $@ $< library.o csvhelper.o

csv2colstore: csv2colstore.cc library.o
	$(CC) -o $@ $< library.o

select2: select2.cc library.o
	$(CC) -o $@ $< library.o

select3: select3.cc library.o
	$(CC) -o $@ $< library.o

clean:
	$(RM) *.o
