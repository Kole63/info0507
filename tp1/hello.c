#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int indice;
    int n_threads;
} kole;

void *afficher(void *arg) {
    kole *k = (kole *)arg;
    printf("Hello world ! du thread %d de %d !\n", k->indice, k->n_threads);
    return NULL;
}
int main(int argc, char* argv[]) {

    if(argc != 2) {
        printf("Veuillez saisir 2 entrées ./hello.o x ou x est les nombres des threads ni plus ni moins\n");
        return -1;
    }

    int n = atoi(argv[1]);

    pthread_t p[n];
    int statut;
    kole k;
    k.n_threads = n;
    for(int i = 0; i <= n; i++) {
        k.indice = i;
        statut = pthread_create(&p[i], NULL, afficher, (void *)&k);
    }
    return 0;
}