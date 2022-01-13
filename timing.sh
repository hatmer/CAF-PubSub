#!/bin/bash

for a in $(seq 1 10); do
	{ time ./main.out ; } 2>> times.txt
done
cat times.txt | grep "real" | cut -f2 | cut -d "m" -f2 | cut -d "s" -f1 > real_times.txt
rm times.txt
cat real_times.txt
