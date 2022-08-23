main : main.o functions.o
	gcc -g -ansi -Wall -pedantic main.o functions.o -o main
main.o : main.c functions.h
	gcc -c -ansi -Wall -pedantic main.c -o main.o
functions.o : functions.c
	gcc -c -ansi -Wall -pedantic functions.c -o functions.o
