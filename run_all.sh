#!/bin/bash

if [ "$1" == "" ]; then
    echo "Usage: $0 filename_of_test_set"
    exit 1;
fi
test_set=$1

if [ -f ./c_test ]; then
    echo ./c_test ${test_set};
    ./c_test ${test_set} | tee answer.log;
fi
if [ -f ./cl_test ]; then 
    echo ./cl_test ${test_set};
    ./cl_test ${test_set} | tee result.log;
fi
if [ -f ./hsa ]; then
    echo ./hsa ${test_set};
    ./hsa ${test_set} | tee result.log;
fi

./valid.sh

