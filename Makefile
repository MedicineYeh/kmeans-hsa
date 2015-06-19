CC=gcc
CFLAGS=-g -Wall -Wno-deprecated-declarations
LFLAGS=-lOpenCL
HSA_RUNTIME_PATH=/opt/hsa
HSA_LLVM_PATH=/opt/amd/cloc/bin
LD_LIBRARY_PATH=$(HSA_RUNTIME_PATH)/lib

.PHONY	:	all execute clean

all	:	data_generator hsa c_test cl_test

execute	:	hsa c_test
	@./run_all.sh test_set

execute_cl	:	cl_test c_test
	@./run_all.sh test_set

cl_test	:	main.c cl_version.c
	@echo "  CC     \033[1;36m$@\033[0;00m"
	@$(CC) main.c -o $@ -D"__USE_CL__" $(CFLAGS) $(LFLAGS)

c_test	:	main.c c_version.c
	@echo "  CC     \033[1;36m$@\033[0;00m"
	@$(CC) main.c -o $@ -D"__USE_C__" $(CFLAGS)

hsa	:	main.c hsa_version.c shader_hsa.o shader_hsa.h
	@echo "  CC     \033[1;36m$@\033[0;00m"
	@$(CC) shader_hsa.o main.c -o $@ -D"__USE_SNACK__" $(CFLAGS) -L$(HSA_RUNTIME_PATH)/lib -lhsa-runtime64

shader_hsa.o	:	shader_hsa.cl
	@echo "  SNACK  \033[1;36m$@\033[0;00m"
	@$(HSA_LLVM_PATH)/snack.sh -q -c $^

data_generator	:	data_generator.c
	@echo "  CC     \033[1;36m$@\033[0;00m"
	@$(CC) $^ -o $@ $(CFLAGS)

clean	:	
	rm cl_test c_test hsa data_generator shader_hsa.o shader_hsa.h
