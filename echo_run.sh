#!/bin/bash
gcc -static -o run run.s
./run
result="$?"
echo "$result"