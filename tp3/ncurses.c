#include <errno.h>
#include <limits.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gsl/gsl_rng.h>

int **mat;
int *mat_stockage;
int *vec;
int *vec_resultat;

static int m;
static int n;
static int intervalle_affichage;
static int valeurs_calculees;
static int demandes_affichage;
static int affichages_effectues;
static int prochain_seuil;
static int nombre_affichages;
static int demarrer_calcul;
static int annuler_calcul;
static int erreur_affichage;
static int *instantanes;

static pthread_mutex_t mutex_etat = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changement_etat = PTHREAD_COND_INITIALIZER;

static WINDOW *fenetre_matrice;
static WINDOW *fenetre_vecteur;
static WINDOW *fenetre_resultat;

typedef struct {
    int debut;
    int taille;
} DonneesThread;

static int lire_entier_positif(const char *texte, int *valeur) {
    char *fin;
    errno = 0;
    long resultat = strtol(texte, &fin, 10);
    if (errno != 0 || texte[0] == '\0' || *fin != '\0' ||
        resultat < 1 || resultat > INT_MAX)
        return 0;
    *valeur = (int)resultat;
    return 1;
}

static int nombre_chiffres(int valeur) {
    int chiffres = 1;
    while (valeur >= 10) {
        valeur /= 10;
        ++chiffres;
    }
    return chiffres;
}

static void liberer_donnees(gsl_rng *generateur) {
    free(mat);
    free(mat_stockage);
    free(vec);
    free(vec_resultat);
    free(instantanes);
    if (generateur != NULL)
        gsl_rng_free(generateur);
}

static int dessiner_resultat(const int *valeurs) {
    if (werase(fenetre_resultat) == ERR ||
        box(fenetre_resultat, 0, 0) == ERR ||
        mvwprintw(fenetre_resultat, 0, 2, "Resultat") == ERR)
        return 0;

    for (int i = 0; i < m; ++i) {
        if (mvwprintw(fenetre_resultat, i + 1, 1, "%d: %d", i, valeurs[i]) == ERR)
            return 0;
    }
    return wnoutrefresh(fenetre_resultat) != ERR && doupdate() != ERR;
}

static int dessiner_donnees_fixes(void) {
    if (werase(fenetre_matrice) == ERR ||
        box(fenetre_matrice, 0, 0) == ERR ||
        mvwprintw(fenetre_matrice, 0, 2, "Matrice") == ERR ||
        werase(fenetre_vecteur) == ERR ||
        box(fenetre_vecteur, 0, 0) == ERR ||
        mvwprintw(fenetre_vecteur, 0, 2, "Vecteur") == ERR)
        return 0;

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            if (mvwprintw(fenetre_matrice, i + 1, 1 + j * 4, "%3d", mat[i][j]) == ERR)
                return 0;
        }
    }
    for (int i = 0; i < n; ++i) {
        if (mvwprintw(fenetre_vecteur, i + 1, 1, "%d: %d", i, vec[i]) == ERR)
            return 0;
    }

    return wnoutrefresh(fenetre_matrice) != ERR &&
           wnoutrefresh(fenetre_vecteur) != ERR &&
           doupdate() != ERR;
}

static void *calcul_bloc(void *argument) {
    DonneesThread *donnees = argument;
    int fin = donnees->debut + donnees->taille;

    pthread_mutex_lock(&mutex_etat);
    while (!demarrer_calcul && !annuler_calcul)
        pthread_cond_wait(&changement_etat, &mutex_etat);
    if (annuler_calcul) {
        pthread_mutex_unlock(&mutex_etat);
        return NULL;
    }
    pthread_mutex_unlock(&mutex_etat);

    for (int i = donnees->debut; i < fin; ++i) {
        int resultat = 0;
        for (int j = 0; j < n; ++j)
            resultat += mat[i][j] * vec[j];

        pthread_mutex_lock(&mutex_etat);
        if (annuler_calcul) {
            pthread_mutex_unlock(&mutex_etat);
            return NULL;
        }

        vec_resultat[i] = resultat;
        ++valeurs_calculees;
        if (valeurs_calculees == prochain_seuil || valeurs_calculees == m) {
            int demande = demandes_affichage++;
            memcpy(instantanes + (size_t)demande * (size_t)m,
                   vec_resultat, (size_t)m * sizeof(*vec_resultat));
            prochain_seuil += intervalle_affichage;
            if (prochain_seuil > m)
                prochain_seuil = m;
            pthread_cond_broadcast(&changement_etat);
        }
        pthread_mutex_unlock(&mutex_etat);
    }
    return NULL;
}

static void *thread_affichage(void *argument) {
    (void)argument;
    for (;;) {
        pthread_mutex_lock(&mutex_etat);
        while (affichages_effectues == demandes_affichage &&
               valeurs_calculees < m && !annuler_calcul)
            pthread_cond_wait(&changement_etat, &mutex_etat);

        if (annuler_calcul ||
            (affichages_effectues == demandes_affichage && valeurs_calculees == m)) {
            pthread_mutex_unlock(&mutex_etat);
            break;
        }

        int demande = affichages_effectues;
        const int *instantane = instantanes + (size_t)demande * (size_t)m;
        pthread_mutex_unlock(&mutex_etat);

        if (!dessiner_resultat(instantane)) {
            pthread_mutex_lock(&mutex_etat);
            erreur_affichage = 1;
            annuler_calcul = 1;
            pthread_cond_broadcast(&changement_etat);
            pthread_mutex_unlock(&mutex_etat);
            break;
        }

        pthread_mutex_lock(&mutex_etat);
        ++affichages_effectues;
        pthread_cond_broadcast(&changement_etat);
        pthread_mutex_unlock(&mutex_etat);
    }
    return NULL;
}

int main(int argc, char **argv) {
    m = 6;
    n = 6;
    int p = 2;
    intervalle_affichage = 1;

    if (argc > 5 ||
        (argc > 1 && !lire_entier_positif(argv[1], &m)) ||
        (argc > 2 && !lire_entier_positif(argv[2], &n))) {
        fprintf(stderr, "Usage: %s [m [n [p [x]]]]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (m < p)
        p = m;
    if ((argc > 3 && !lire_entier_positif(argv[3], &p)) ||
        (argc > 4 && !lire_entier_positif(argv[4], &intervalle_affichage)) ||
        p > m || intervalle_affichage > m) {
        fprintf(stderr, "p et x doivent etre compris entre 1 et m.\n");
        return EXIT_FAILURE;
    }
    if ((size_t)m > SIZE_MAX / (size_t)n / sizeof(*mat_stockage)) {
        fprintf(stderr, "Dimensions trop grandes.\n");
        return EXIT_FAILURE;
    }
    nombre_affichages = 1 + (m - 1) / intervalle_affichage;
    if ((size_t)m > SIZE_MAX / (size_t)nombre_affichages / sizeof(*instantanes)) {
        fprintf(stderr, "Trop d'instantanes pour ces dimensions et ce seuil.\n");
        return EXIT_FAILURE;
    }
    instantanes = malloc((size_t)m * (size_t)nombre_affichages * sizeof(*instantanes));
    if (instantanes == NULL) {
        fprintf(stderr, "Impossible d'allouer les instantanes d'affichage.\n");
        return EXIT_FAILURE;
    }

    gsl_rng_env_setup();
    gsl_rng *generateur = gsl_rng_alloc(gsl_rng_default);
    if (generateur == NULL) {
        fprintf(stderr, "Impossible d'allouer le generateur aleatoire.\n");
        free(instantanes);
        return EXIT_FAILURE;
    }
    gsl_rng_set(generateur, (unsigned long)time(NULL));

    mat = malloc((size_t)m * sizeof(*mat));
    mat_stockage = malloc((size_t)m * (size_t)n * sizeof(*mat_stockage));
    vec = malloc((size_t)n * sizeof(*vec));
    vec_resultat = malloc((size_t)m * sizeof(*vec_resultat));
    if (mat == NULL || mat_stockage == NULL || vec == NULL || vec_resultat == NULL) {
        fprintf(stderr, "Impossible d'allouer les donnees de calcul.\n");
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < m; ++i)
        mat[i] = mat_stockage + (size_t)n * (size_t)i;
    for (size_t i = 0; i < (size_t)m * (size_t)n; ++i)
        mat_stockage[i] = (int)(gsl_rng_get(generateur) % 5) + 1;
    for (int i = 0; i < n; ++i)
        vec[i] = (int)(gsl_rng_get(generateur) % 5) + 1;
    for (int i = 0; i < m; ++i)
        vec_resultat[i] = -1;

    if (initscr() == NULL) {
        fprintf(stderr, "Impossible d'initialiser ncurses.\n");
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }
    cbreak();
    noecho();
    curs_set(0);

    int lignes_ecran;
    int colonnes_ecran;
    getmaxyx(stdscr, lignes_ecran, colonnes_ecran);
    long long largeur_matrice_longue = 4LL * n + 2;
    long long hauteur_matrice_longue = (long long)m + 2;
    long long hauteur_vecteurs_longue = (long long)(m > n ? m : n) + 2;
    if (largeur_matrice_longue > colonnes_ecran ||
        hauteur_matrice_longue + hauteur_vecteurs_longue > lignes_ecran) {
        endwin();
        fprintf(stderr, "Terminal trop petit pour ces dimensions.\n");
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }

    int hauteur_matrice = (int)hauteur_matrice_longue;
    int hauteur_vecteurs = (int)hauteur_vecteurs_longue;
    int largeur_matrice = largeur_matrice_longue < 10
                              ? 10 : (int)largeur_matrice_longue;
    int largeur_vecteur = nombre_chiffres(m - 1) + nombre_chiffres(5) + 4;
    if (largeur_vecteur < 12)
        largeur_vecteur = 12;
    long long maximum_resultat_long = 25LL * n;
    if (maximum_resultat_long > INT_MAX) {
        endwin();
        fprintf(stderr, "Dimensions trop grandes pour le type de resultat.\n");
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }
    int maximum_resultat = (int)maximum_resultat_long;
    int largeur_resultat = nombre_chiffres(m - 1) +
                           (maximum_resultat < 10 ? 2 : nombre_chiffres(maximum_resultat)) + 5;
    if (largeur_resultat < 16)
        largeur_resultat = 16;

    if (largeur_matrice > colonnes_ecran ||
        largeur_vecteur + largeur_resultat > colonnes_ecran ||
        (long long)hauteur_matrice + hauteur_vecteurs > lignes_ecran) {
        endwin();
        fprintf(stderr, "Terminal trop petit pour ces dimensions (%d lignes, %d colonnes requis au minimum).\n",
                hauteur_matrice + hauteur_vecteurs,
                largeur_matrice > largeur_vecteur + largeur_resultat
                    ? largeur_matrice : largeur_vecteur + largeur_resultat);
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }

    fenetre_matrice = newwin(hauteur_matrice, largeur_matrice, 0, 0);
    fenetre_vecteur = newwin(hauteur_vecteurs, largeur_vecteur, hauteur_matrice, 0);
    fenetre_resultat = newwin(hauteur_vecteurs, largeur_resultat,
                              hauteur_matrice, largeur_vecteur);
    if (fenetre_matrice == NULL || fenetre_vecteur == NULL || fenetre_resultat == NULL ||
        !dessiner_donnees_fixes() || !dessiner_resultat(vec_resultat)) {
        if (fenetre_matrice != NULL)
            delwin(fenetre_matrice);
        if (fenetre_vecteur != NULL)
            delwin(fenetre_vecteur);
        if (fenetre_resultat != NULL)
            delwin(fenetre_resultat);
        endwin();
        fprintf(stderr, "Impossible de creer ou dessiner les fenetres ncurses.\n");
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }

    pthread_t *threads = malloc((size_t)p * sizeof(*threads));
    DonneesThread *donnees = malloc((size_t)p * sizeof(*donnees));
    if (threads == NULL || donnees == NULL) {
        delwin(fenetre_matrice);
        delwin(fenetre_vecteur);
        delwin(fenetre_resultat);
        endwin();
        fprintf(stderr, "Impossible d'allouer les donnees des threads.\n");
        free(threads);
        free(donnees);
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }

    prochain_seuil = intervalle_affichage;
    pthread_t afficheur;
    int erreur = pthread_create(&afficheur, NULL, thread_affichage, NULL);
    if (erreur != 0) {
        delwin(fenetre_matrice);
        delwin(fenetre_vecteur);
        delwin(fenetre_resultat);
        endwin();
        fprintf(stderr, "Impossible de creer le thread d'affichage (erreur %d).\n", erreur);
        free(threads);
        free(donnees);
        liberer_donnees(generateur);
        return EXIT_FAILURE;
    }

    int base = m / p;
    int reste = m % p;
    int debut = 0;
    int threads_crees = 0;
    for (int i = 0; i < p; ++i) {
        donnees[i].debut = debut;
        donnees[i].taille = base + (i < reste ? 1 : 0);
        debut += donnees[i].taille;
        erreur = pthread_create(&threads[i], NULL, calcul_bloc, &donnees[i]);
        if (erreur != 0) {
            fprintf(stderr, "Impossible de creer le thread de calcul %d (erreur %d).\n", i, erreur);
            break;
        }
        ++threads_crees;
    }

    if (threads_crees != p) {
        pthread_mutex_lock(&mutex_etat);
        annuler_calcul = 1;
        pthread_cond_broadcast(&changement_etat);
        pthread_mutex_unlock(&mutex_etat);
    } else {
        pthread_mutex_lock(&mutex_etat);
        demarrer_calcul = 1;
        pthread_cond_broadcast(&changement_etat);
        pthread_mutex_unlock(&mutex_etat);
    }

    for (int i = 0; i < threads_crees; ++i) {
        erreur = pthread_join(threads[i], NULL);
        if (erreur != 0) {
            fprintf(stderr, "Impossible de joindre le thread %d (erreur %d).\n", i, erreur);
            return EXIT_FAILURE;
        }
    }
    erreur = pthread_join(afficheur, NULL);
    if (erreur != 0) {
        fprintf(stderr, "Impossible de joindre le thread d'affichage (erreur %d).\n", erreur);
        return EXIT_FAILURE;
    }

    int calcul_complet = threads_crees == p && !erreur_affichage &&
                        valeurs_calculees == m &&
                        affichages_effectues == nombre_affichages;

    /* Attendre que l'utilisateur appuie sur une touche */
    if (calcul_complet) {
        mvprintw(LINES - 1, 0, "Calcul termine. Appuyez sur une touche pour quitter.");
        refresh();
        getch();
    }

    delwin(fenetre_matrice);
    delwin(fenetre_vecteur);
    delwin(fenetre_resultat);
    endwin();

    if (erreur_affichage)
        fprintf(stderr, "Erreur pendant l'actualisation de la fenetre ncurses.\n");
    int resultat_verifie = calcul_complet;
    for (int i = 0; resultat_verifie && i < m; ++i) {
        int attendu = 0;
        for (int j = 0; j < n; ++j)
            attendu += mat[i][j] * vec[j];
        if (vec_resultat[i] != attendu)
            resultat_verifie = 0;
    }
    printf("Verification du resultat : %s\n",
           resultat_verifie ? "reussie" : "echouee");

    free(threads);
    free(donnees);
    liberer_donnees(generateur);
    return resultat_verifie ? EXIT_SUCCESS : EXIT_FAILURE;
}
