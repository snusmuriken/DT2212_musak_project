#!/bin/bash
# Compile cpp file to sfml-app

CFILE=$1
OUTFILE=$2

/usr/bin/g++ -c $CFILE -o temp.o
/usr/bin/g++ temp.o -o $OUTFILE -lsfml-graphics -lsfml-window -lsfml-system

