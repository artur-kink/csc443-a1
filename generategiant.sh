#!/bin/bash

python mkcsv.py genlots.csv 5000
cat genlots.csv > lots.csv
while [ $(stat -f %z lots.csv) -lt 1050000000 ]
do
    cat genlots.csv >> lots.csv
done
