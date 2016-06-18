#nvcc -I ./ gpu-match.cu main.c parser.c test.c free.c -o main
gcc -g -I ./ main.c parser.c test.c free.c -o main-cpu

