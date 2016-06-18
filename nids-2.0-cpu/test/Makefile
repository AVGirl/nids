LDFLAGS = -L. -lnids -ldl -lpthread -lrt
main:main.o
	gcc -g -o $@ $^ $(LDFLAGS)
main.o:main.c
	gcc -g -o $@ -c $^ -I.
clean:
	rm -f *.o main

