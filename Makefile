CC=gcc
CFLAGS=-g -Wall
LFLAGS=-lOpenCL
HSA_RUNTIME_PATH=/opt/hsa
HSA_LLVM_PATH=/opt/amd/cloc/bin
LD_LIBRARY_PATH=$(HSA_RUNTIME_PATH)/lib

all	:	cl_test

execute	:	hsa
	./hsa

cl_test	:	main.c cl_version.c
	$(CC) main.c -o $@ -D"__USE_CL__" $(CFLAGS) $(LFLAGS)

c_test	:	main.c c_version.c
	$(CC) main.c -o $@ -D"__USE_C__" $(CFLAGS)

hsa	:	main.c hsa_version.c shader_hsa.o shader_hsa.h
	$(CC) shader_hsa.o main.c -o $@ -D"__USE_SNACK__" $(CFLAGS) -L$(HSA_RUNTIME_PATH)/lib -lhsa-runtime64

shader_hsa.o	:	shader_hsa.cl
	$(HSA_LLVM_PATH)/snack.sh -q -c $^

clean	:	
	rm cl_test
