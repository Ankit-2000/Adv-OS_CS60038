main: main.o disc.o sfs.o
	gcc -o main main.o disc.o sfs.o 
main.o: main.c disc.h sfs.h
	gcc -c -g main.c
sfs.o: sfs.c sfs.h
	gcc -c -g sfs.c
disc.o: disc.c disc.h
	gcc -c -g disc.c

clean:
	rm disc.o sfs.o main.o main  
