#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define SPIN 400000000

int compteur = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* incrementer(void* arg)
{
    for (int i = 0; i < SPIN / 4; i++) {
        pthread_mutex_lock(&mutex);
        compteur++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
	int diff_temps;
	time_t temps_1, temps_2;

	temps_1 = time(NULL);
	pthread_t p[4];
    int statut;

    for(int i = 0; i < 4; i++) {
        statut = pthread_create(&p[i], NULL, incrementer, NULL);
    }

    for(int i = 0; i < 4; i++) {
        pthread_join(p[i],NULL);
    }

	temps_2 = time(NULL);
	diff_temps = (int) temps_2 - (int) temps_1;
	printf("Valeur finale du compteur : %d\n", compteur);
	printf("Temps : %d secondes\n", diff_temps);

	return EXIT_SUCCESS;
}