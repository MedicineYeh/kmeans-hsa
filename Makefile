CC=gcc
CFLAGS=-g -Wall
LFLAGS=-lOpenCL

all	:	cl_test

execute	:	cl_test
	./cl_test

cl_test	:	main.c cl_version.c  c_version.c
	$(CC) $(CFLAGS) $(LFLAGS) main.c -o $@

clean	:	
	rm cl_test
