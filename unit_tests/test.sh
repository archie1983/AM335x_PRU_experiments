#!/bin/sh

echo "Compiling tests:"
rm test_queue
rm ae_queue.o

gcc -c ../ae_queue.c
gcc ae_queue.o test_queue.c -o test_queue

echo "Running tests:"
./test_queue
