main : main.o threadpool.o 
	gcc main.o threadpool.o -l pthread -o main
main.o : main.c
	gcc -c main.c -o main.o
threadpool.o : threadpool.c
	gcc -c threadpool.c -o threadpool.o

clean:
	rm *.o main