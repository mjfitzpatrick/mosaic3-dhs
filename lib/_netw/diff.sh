#!/bin/sh
for F in `ls dca*.[ch]`; do
 diff    $F ../_netw.20080806/$F >> dca.txt 2>&1
 diff -y $F ../_netw.20080806/$F >> dca.dat 2>&1
done
for F in `ls dhs*.[ch]`; do
 diff    $F ../_netw.20080806/$F >> dhs.txt 2>&1
 diff -y $F ../_netw.20080806/$F >> dhs.dat 2>&1
done
