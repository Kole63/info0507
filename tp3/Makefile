all: compile run

compile: exercice1.c
	gcc exercice1.c -o main -lgsl
run:
	./main 100 10
ex2: exercice2.c
	gcc exercice2.c -o main -lgsl -pthread
	./main
ncurses :
	gcc ncurses.c -o main -lgsl -lncurses -pthread
	./main $(ARGS)
clean:
	rm -f main