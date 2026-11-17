#define M	6
#define N	6
#include <gsl/gsl_rng.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include <ncurses.h>

int **mat;
int *mat_stockage;
int *vec;
int *vec_resultat;

void afficher_matrice(int **mat, size_t m, size_t n) {
	printf("*****Matrice******\n");
	for (size_t i = 0; i < m; ++i){
		for (size_t j = 0; j < n; ++j)
			printf("%d ", mat[i][j]);
		printf("\n");
	}
	printf("**************\n");
}

void afficher_vecteur(int *vec, size_t n) {
	printf("*****Vecteur******\n");
	for (size_t i = 0; i < n; ++i)
		printf("%d ", vec[i]);
	printf("\n");
	printf("*************\n");
}
typedef struct {
    int start;
    int block_size;
} ThreadData;

void* calcul_bloc(void* args) {
    ThreadData *data = (ThreadData*)args;
    int end = data->start + data->block_size;
    for (int i = data->start; i < end; ++i) {
        vec_resultat[i] = 0;
        for (int j = 0; j < N; ++j)
            vec_resultat[i] += mat[i][j] * vec[j];
    }
    return NULL;
}

int main() {
    int p;
    printf("Donnez le nombre de threads p (diviseur de M = %d) : ", M);
    if (scanf("%d", &p) != 1 || p <= 0 || p > M || M % p != 0) {
        fprintf(stderr, "Le nombre de threads doit etre un diviseur positif de M.\n");
        return EXIT_FAILURE;
    }

	gsl_rng_env_setup();
	const gsl_rng_type *T = gsl_rng_default;
	gsl_rng *r = gsl_rng_alloc (T);

    if (r == NULL) {
        fprintf(stderr, "Impossible d'allouer le generateur aleatoire.\n");
        return EXIT_FAILURE;
    }
    gsl_rng_set(r, time(NULL));

	mat_stockage = malloc(sizeof(int[M * N]));
	mat = malloc(sizeof(int *[M]));
	vec = malloc(sizeof(int[N]));
	for (size_t i = 0; i < M; ++i)
		mat[i] = mat_stockage + N * i;
	for (size_t i = 0; i < M; ++i)
		for (size_t j = 0; j < N; ++j)
			mat[i][j] = (gsl_rng_get(r) % 5) + 1;

	for (size_t i = 0; i < N; ++i)
		vec[i] = (gsl_rng_get(r) % 5) + 1;

	vec_resultat = malloc(sizeof(int[M]));
	for (size_t i = 0; i < M; ++i)
		vec_resultat[i] = -1;	

	afficher_matrice(mat, M, N);
	afficher_vecteur(vec, N);

    pthread_t *threads = malloc((size_t)p * sizeof(*threads));
    ThreadData *data = malloc((size_t)p * sizeof(*data));
    if (threads == NULL || data == NULL) {
        fprintf(stderr, "Impossible d'allouer les donnees des threads.\n");
        free(threads);
        free(data);
        free(vec);
        free(vec_resultat);
        free(mat_stockage);
        free(mat);
        gsl_rng_free(r);
        return EXIT_FAILURE;
    }

    int bloc = M / p;
    int threads_crees = 0;
    for (int i = 0; i < p; ++i) {
        data[i].start = i * bloc;
        data[i].block_size = bloc;
        int erreur = pthread_create(&threads[i], NULL, calcul_bloc, &data[i]);
        if (erreur != 0) {
            fprintf(stderr, "Impossible de creer le thread %d (erreur %d).\n", i, erreur);
            break;
        }
        ++threads_crees;
    }

    int succes = threads_crees == p;
    for (int i = 0; i < threads_crees; ++i) {
        int erreur = pthread_join(threads[i], NULL);
        if (erreur != 0) {
            fprintf(stderr, "Impossible de joindre le thread %d (erreur %d).\n", i, erreur);
            return EXIT_FAILURE;
        }
    }

    if (!succes) {
        free(threads);
        free(data);
        free(vec);
        free(vec_resultat);
        free(mat_stockage);
        free(mat);
        gsl_rng_free(r);
        return EXIT_FAILURE;
    }

	afficher_vecteur(vec_resultat, M);

    int resultat_verifie = 1;
    for (int i = 0; i < M; ++i) {
        int resultat_sequentiel = 0;
        for (int j = 0; j < N; ++j)
            resultat_sequentiel += mat[i][j] * vec[j];
        if (vec_resultat[i] != resultat_sequentiel) {
            resultat_verifie = 0;
            break;
        }
    }
    printf("Verification du resultat : %s\n", resultat_verifie ? "reussie" : "echouee");

    free(threads);
    free(data);
	free(vec);
	free(vec_resultat);
	free(mat_stockage);
	free(mat);
	gsl_rng_free (r);

	return resultat_verifie ? EXIT_SUCCESS : EXIT_FAILURE;
}