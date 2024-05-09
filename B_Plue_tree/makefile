gcomplier = gcc
gflag=-std=c99 -w -g -I.
ofiles =  btree_mgr.o dberror.o expr.o record_mgr.o rm_serializer.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o
hfiles = dberror.h expr.h record_mgr.h tables.h test_helper.h btree_mgr.h 

default: test1

test1: test_assign4_1.o $(ofiles)
	$(gcomplier) $(gflag) -o test1 test_assign4_1.o $(ofiles) -lm
test2: test_assign4_2.o $(ofiles)
	$(gcomplier) $(gflag) -o test2 test_assign4_2.o $(ofiles)

test_assign4_2.o: test_assign4_2.c $(hfiles) buffer_mgr.h
	$(gcomplier) $(gflag) -c test_assign4_2.c -lm
	
test_assign4_1.o: test_assign4_1.c $(hfiles) buffer_mgr.h
	$(gcomplier) $(gflag) -c test_assign4_1.c -lm

btree_mgr.o: btree_mgr.c $(hfiles)
	$(gcomplier) $(gflag) -c btree_mgr.c

record_mgr.o: record_mgr.c record_mgr.h buffer_mgr.h storage_mgr.h
	$(gcomplier) $(gflag) -c  record_mgr.c

expr.o: expr.c dberror.h record_mgr.h expr.h tables.h
	$(gcomplier) $(gflag) -c expr.c

rm_serializer.o: rm_serializer.c dberror.h tables.h record_mgr.h
	$(gcomplier) $(gflag) -c rm_serializer.c

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	$(gcomplier) $(gflag) -c buffer_mgr_stat.c

buffer_mgr.o: buffer_mgr.c buffer_mgr.h dt.h storage_mgr.h
	$(gcomplier) $(gflag) -c buffer_mgr.c

storage_mgr.o: storage_mgr.c storage_mgr.h 
	$(gcomplier) $(gflag) -c storage_mgr.c -lm

dberror.o: dberror.c dberror.h 
	$(gcomplier) $(gflag) -c dberror.c

clean: 
	$(RM) test1  *.o *~

run_test1:
	./test1

