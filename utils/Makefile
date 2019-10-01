all: utils.o

utils.o: utils.cc
	g++ -std=c++11 $^ -c -o $@

clean::
	rm -fv utils.o
