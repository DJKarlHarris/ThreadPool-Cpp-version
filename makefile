main : main.o ThreadPool.o TaskQueue.o
	g++ main.o ThreadPool.o TaskQueue.o -l pthread -o main
main.o : main.cpp
	g++ -c main.cpp -o main.o
ThreadPool.o : ThreadPool.cpp
	g++ -c ThreadPool.cpp -o ThreadPool.o
TaskQueue.o : TaskQueue.cpp
	g++ -c TaskQueue.cpp -o TaskQueue.o

clean:
	rm *.o main