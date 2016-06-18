INCLUDEFLAG = /usr/local/cuda/include 

libnids.a: gpu-match.o parser.o free.o test.o libcudart_static.a.o
	ar rcs libnids.a gpu-match.o parser.o free.o test.o libcudart_static.a.o
	rm -f test/$@
	cp $@ test

#libnids.o: gpu-match.cu parser.c free.c test.c 
#	nvcc -c gpu-match.cu parser.c free.c test.c -o libnids.o
#	ar x libcudart_static.a

gpu-match.o: gpu-match.cu
	nvcc -I. -c gpu-match.cu -o gpu-match.o
	ar x libcudart_static.a
parser.o: parser.c 
	nvcc -I. -c parser.c -o parser.o
free.o: free.c
	nvcc -I. -c free.c -o free.o
test.o: test.c 
	nvcc -I. -c test.c -o test.o
clean:
	rm -f *.o libnids.a
