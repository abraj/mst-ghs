all:
	clear; echo "Building sources..."; g++ src/*.cpp -lpthread -o bin/out; clear; echo ":: Usage ::\n make [run|clean]\n";

run:
	bin/./out;

clean:
	rm *~ src/*~ io/*~ bin/* io/output.txt; clear;
