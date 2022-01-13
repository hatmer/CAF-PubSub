#!/bin/bash
#max-threads=$a;
#for a in $(seq 1 2)
#do
#	{ time ./main.out ; } 2>> times.txt
#done
#-10-100-1000-100.out -10-500-1000-100.out
# for b in $(seq 1 6)
# do
	# for a in  Push-10-100-10000-100.out Push-10-500-10000-100.out
		# do
			# { time ./main$a ; } 2>> times.txt
		# done
# done

for b in $(seq 1 6)
do
	##for a in  Push-10-100-10000-100.out Push-10-500-10000-100.out
		##do
		
			{ time ./main-10-500-1000-100.out ; } 2>> times.txt
		#done
done
for b in $(seq 1 6)
do
	{ time ./main-10-100-1000-100.out ; } 2>> times.txt
done
cat times.txt | grep "real" | cut -f2 | cut -d "m" -f2 | cut -d "s" -f1 > real_times.txt
rm times.txt
cat real_times.txt
