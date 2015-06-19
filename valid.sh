#!/bin/bash

echo ""
echo ""
if [ "$(diff answer.log result.log)"  == "" ]; then 
    echo -e "\033[1;32mPass Validation\033[0;00m";
else
    echo -e "\033[1;41mFail Validation\033[0;00m";
fi
echo ""
echo ""
echo ""
