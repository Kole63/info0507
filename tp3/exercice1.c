#include <gsl/gsl_rng.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

int *tableau;
void initialiser_t_random(int n) {
    gsl_rng_env_setup();
    const gsl_rng_type *T = gsl_rng_default;
    gsl_rng *r = gsl_rng_alloc (T);
    gsl_rng_set(r, time(NULL));
    for (int i = 0; i < n; i++) {
        tableau[i] = gsl_rng_get(r) % 6 + 1 ; // Remplir le tableau avec des entiers aléatoires entre 1 et 6
    }

    gsl_rng_free (r);
}
typedef struct {
    int start;
    int end;
    int thread_id;
} ThreadData;
void* somme_partielle(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int start = data->start;
    int end = data->end;
    int thread_id = data->thread_id;

    int somme_locale = 0;
    for (int i = start; i < end; i++) {
        somme_locale += tableau[i];
    }

    printf("Thread %d: somme partielle = %d\n", thread_id, somme_locale);
    return (void*)(intptr_t)somme_locale;
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Veuillez saisir la taille du tableau et le nombre de threads\n");
        return EXIT_FAILURE;
    }
    int n = atoi(argv[1]);
    int nb_threads = atoi(argv[2]);

    tableau = malloc(n * sizeof(int));
    if (tableau == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        return EXIT_FAILURE;
    }
    initialiser_t_random(n);

    int somme = 0;
    if (nb_threads > n) {
        nb_threads = n; // Limiter le nombre de threads à la taille du tableau
    }
    pthread_t threads[nb_threads];
    ThreadData thread_data[nb_threads];

    int chunk_size = n / nb_threads;
    for (int i = 0; i < nb_threads; i++) {
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == nb_threads - 1) ? n : (i + 1) * chunk_size;
        thread_data[i].thread_id = i;

        pthread_create(&threads[i], NULL, somme_partielle, &thread_data[i]);
    }

    for (int i = 0; i < nb_threads; i++) {
        void* somme_locale;
        pthread_join(threads[i], &somme_locale);
        somme += (intptr_t)somme_locale;
    }
    printf("Somme totale = %d\n", somme);

    free(tab);
    
    return EXIT_SUCCESS;
}