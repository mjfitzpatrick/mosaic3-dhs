#!/bin/sh
for F in `ls *.[ch]`; do
 diff $F /home/monsoon/src/Util/dhsLibraries/_netw/$F >> differences.txt 2>&1
 diff -y $F /home/monsoon/src/Util/dhsLibraries/_netw/$F >> differences.dat 2>&1
done
