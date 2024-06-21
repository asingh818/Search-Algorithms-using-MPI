#!/bin/bash
for file in *_in.txt; do
	echo $file;
	outfile=${file/_in/_out};
	(mpiexec -n 4 MPI.exe < $file) > $outfile;
done
