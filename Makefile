CC = gcc
FLAGS = -Wall -g -lm
PROG = demoTFS
OBJS = demoTFS.o libTinyFS.o libDisk.o

$(PROG): $(OBJS)
	$(CC) $(FLAGS) -o $(PROG) $(OBJS)

demoTFS.o: demoTFS.c libTinyFS.h libTinyFS.h TinyFS_errno.h
	$(CC) $(FLAGS) -c -o $@ $<

libTinyFS.o: libTinyFS.c libTinyFS.h libTinyFS.h libDisk.h libDisk.o TinyFS_errno.h
	$(CC) $(FLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h libTinyFS.h TinyFS_errno.h
	$(CC) $(FLAGS) -c -o $@ $<
