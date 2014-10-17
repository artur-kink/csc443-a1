#!/bin/bash

if [ $# -lt 2 ]
    then
        echo "Usage: $0 <filename>"
        exit 1
fi

python mkcsv.py _temp.csv 5000
cat genlots.csv > $1
while [ $(stat -f %z $1) -lt 1050000000 ]
do
    cat _temp.csv >> $1
done

rm _temp.csv
