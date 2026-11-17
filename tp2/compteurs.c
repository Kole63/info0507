#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <ncurses.h>

int nanosleep(const struct timespec *duration, struct timespec * rem);

#define NB_LIGNES_FEN_C1		4			/* Dimensions des fenetres du programme */
#define NB_COL_FEN_C1			24
#define NB_LIGNES_FEN_C2		4
#define NB_COL_FEN_C2			24
#define NB_LIGNES_FEN_C3		4
#define NB_COL_FEN_C3			24
#define NB_LIGNES_FEN_BOUTONS	11
#define NB_COL_FEN_BOUTONS		72

WINDOW *fen_c1;
WINDOW *fen_c2;
WINDOW *fen_c3;
WINDOW *fen_boutons;

void ncurses_initialiser() {
	initscr();								/* Demarre le mode ncurses */
	cbreak();								/* Pour les saisies clavier (desac. mise en buffer) */
	noecho();								/* Desactive l'affichage des caracteres saisis */
	keypad(stdscr, TRUE);					/* Active les touches specifiques */
	refresh();								/* Met a jour l'affichage */
	curs_set(FALSE);						/* Masque le curseur */
	mousemask(BUTTON1_CLICKED, NULL);		/* Active le clic gauche de la souris*/
}

void ncurses_stopper() {
	endwin();
}

void creer_fenetres() {	
	fen_c1 = newwin(NB_LIGNES_FEN_C1, NB_COL_FEN_C1, 0, 0);
	box(fen_c1, 0, 0);
	mvwprintw(fen_c1, 0, NB_COL_FEN_C1 / 2 - 4, "COMPTEUR 1");	
	wrefresh(fen_c1);

	fen_c2 = newwin(NB_LIGNES_FEN_C2, NB_COL_FEN_C2, 0, NB_COL_FEN_C1);
	box(fen_c2, 0, 0);
	mvwprintw(fen_c2, 0, NB_COL_FEN_C2 / 2 - 4, "COMPTEUR 2");	
	wrefresh(fen_c2);

	fen_c3 = newwin(NB_LIGNES_FEN_C3, NB_COL_FEN_C3, 0, NB_COL_FEN_C1 + NB_COL_FEN_C2);
	box(fen_c3, 0, 0);
	mvwprintw(fen_c3, 0, NB_COL_FEN_C3 / 2 - 4, "COMPTEUR 3");	
	wrefresh(fen_c3);

	fen_boutons = newwin(NB_LIGNES_FEN_BOUTONS, NB_COL_FEN_BOUTONS, NB_LIGNES_FEN_C1, 0);
	box(fen_boutons, 0, 0);
	mvwprintw(fen_boutons, 0, NB_COL_FEN_BOUTONS / 2 - 4, "BOUTONS");
	mvwprintw(fen_boutons, 2, NB_COL_FEN_BOUTONS / 2 - 5, "Executer");
	mvwprintw(fen_boutons, 4, NB_COL_FEN_BOUTONS / 2 - 5, "Suspendre");
	mvwprintw(fen_boutons, 6, 1, "  Executer compteur 1     Executer compteur 2     Executer compteur 3");
	mvwprintw(fen_boutons, 8, 1, "  Suspendre compteur 1    Suspendre compteur 2    Suspendre compteur 3");
	wrefresh(fen_boutons);
}

void detruire_fenetres() {
	delwin(fen_c1);
	delwin(fen_c2);
	delwin(fen_c3);
	delwin(fen_boutons);
}

int main() {
	MEVENT event;
	int ch;
	struct timespec temps_sleep = {
		.tv_sec = 0,
		.tv_nsec = 500000000
	};
	size_t i;
	
	ncurses_initialiser();
	creer_fenetres();
	
	mvprintw(LINES - 1, 0, "Tapez F2 pour quitter");
	wrefresh(stdscr);

	i = 0;
	while (i < 6) {
		++i;
		mvwprintw(fen_c1, 1, 12, "%zu", i);
		wrefresh(fen_c1);
		nanosleep(&temps_sleep, NULL);
	}
	i = 0;
	while (i < 6) {
		++i;
		mvwprintw(fen_c2, 1, 12, "%zu", i);
		wrefresh(fen_c2);
		nanosleep(&temps_sleep, NULL);
	}
	i = 0;
	while (i < 6) {
		++i;
		mvwprintw(fen_c3, 1, 12, "%zu", i);
		wrefresh(fen_c3);
		nanosleep(&temps_sleep, NULL);
	}

	while((ch = getch()) != KEY_F(2)) {
		switch(ch) {
			case KEY_MOUSE :
				if (getmouse(&event) == OK) {
					if (event.y == 6 && event.x >= 31 && event.x <= 38) {
						mvwprintw(fen_c1, 2, 8, "Execution");
						mvwprintw(fen_c2, 2, 8, "Execution");
						mvwprintw(fen_c3, 2, 8, "Execution");
						wrefresh(fen_c1);
						wrefresh(fen_c2);
						wrefresh(fen_c3);
					}
					else if (event.y == 8 && event.x >= 31 && event.x <= 39) {
						mvwprintw(fen_c1, 2, 8, "  Pause  ");
						mvwprintw(fen_c2, 2, 8, "  Pause  ");
						mvwprintw(fen_c3, 2, 8, "  Pause  ");
						wrefresh(fen_c1);
						wrefresh(fen_c2);
						wrefresh(fen_c3);
					}
					else if (event.y == 10 && event.x >= 3 && event.x <= 21) {
						mvwprintw(fen_c1, 2, 8, "Execution");
						wrefresh(fen_c1);
					}
					else if (event.y == 10 && event.x >= 27 && event.x <= 45) {
						mvwprintw(fen_c2, 2, 8, "Execution");
						wrefresh(fen_c2);
					}
					else if (event.y == 10 && event.x >= 51 && event.x <= 69) {
						mvwprintw(fen_c3, 2, 8, "Execution");
						wrefresh(fen_c3);
					} 
					else if (event.y == 12 && event.x >= 3 && event.x <= 22) {
						mvwprintw(fen_c1, 2, 8, "  Pause  ");
						wrefresh(fen_c1);
					}
					else if (event.y == 12 && event.x >= 27 && event.x <= 46) {
						mvwprintw(fen_c2, 2, 8, "  Pause  ");
						wrefresh(fen_c2);
					}
					else if (event.y == 12 && event.x >= 51 && event.x <= 70) {
						mvwprintw(fen_c3, 2, 8, "  Pause  ");
						wrefresh(fen_c3);
					}
				}
			break;
		}
	}

	detruire_fenetres();
	ncurses_stopper();

	return EXIT_SUCCESS;
}