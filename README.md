# This is for homework

# Data Preparation
`./data_generator test_set 100000 10`

# Execution on HSA Machine
* Manual Compilation: `make c_test hsa data_generator`
* Automatic Compilation and Execution: `make execute`

# Execution on OpenCL Machine
* Manual Compilation: `make c_test cl_test data_generator`
* Automatic Compilation and Execution: `make execute_cl`

# Manual Execution and Validation
`./run_all.sh`

# Arguments Description
* `./data_generator [FILE TO EXPORT] [DCNT] [DIM]`
* `./c_test [FILE TO EXPORT]`
* Notice that the number of _DCNT_ and _DIM_ must be matched to the `#define N_DCNT` and `#define N_DIM` in main.c

