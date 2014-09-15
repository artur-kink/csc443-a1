# Makefile
CC = g++
     
library.o: library.cc library.h
	o  -c $<

library: library.o
  
csv2heapfile: csv2heapfile.cc library.o
	-o  $< library.o
     
scan: scan.cc library.o
	-o  $< library.o
     
insert: insert.cc library.o
	-o  $< library.o
     
update: update.cc library.o
	-o  $< library.o
     
delete: delete.cc library.o
	-o  $< library.o
     
select: select.cc library.o
	-o  $< library.o
     
csv2colstore: csv2colstore.cc library.o
	-o  $< library.o
     
select2: select2.cc library.o
	-o  $< library.o
     
select3: select3.cc library.o
	-o  $< library.o
