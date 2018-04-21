FILES = main.cpp Message.cpp Message.h

all:
	g++ -g -std=c++11 -o main $(FILES) -pthread -lncurses

clean:
	-rm -f *.o
	-rm -f main
