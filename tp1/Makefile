all: run
hello.o: hello.c
	gcc hello.c -o hello.o
run: hello.o
	./hello.o 6
clean:
	rm -f hello.o