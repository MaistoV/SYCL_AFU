#!/bin/bash

for file in data_PLAIN_C/*; do
    echo $file > tmp
    sed -i "s/SYCL_ASP/PLAIN_C/g" tmp
    cat tmp | xargs mv $file
done
rm tmp