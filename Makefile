main: main.cc
	g++ -Werror -Wall -pthread main.cc -o main
.PHONY: run
run: main
	./main 4 plik1.txt plik2.txt plik3.txt plik4.txt plik5.txt
