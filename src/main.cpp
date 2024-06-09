#include <clocale>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>

static void finish(int sig);

void drawPipe(int xPos, int height) {
	if (height > 0) {
		for (int i = 0; i < height; i++) {
			mvaddch(i, xPos, '|');
		}
	} else if (height < 0) {
		for (int i = LINES; i > LINES + height; i--) {
			mvaddch(i, xPos, '|');
		}
	} else {
		return;
	}
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    signal(SIGINT, finish);      // arrange interrupts to terminate

	setlocale(LC_ALL, "");

    initscr(); // initialize the curses library
    cbreak(); // take input chars one at a time, no wait for \n
	noecho(); // don't echo output
	nodelay(stdscr, true); // don't wait for keypresses if there aren't any

    if (has_colors()) {
        start_color();

        init_pair(1, COLOR_GREEN, -1);
        init_pair(2, COLOR_YELLOW, -1);
		#define PIPE_COLOR 1
		#define BIRD_COLOR 2
    }

	int num = 1;
    while (true) {
		drawPipe(num, (num * 10) % (2 * LINES) - LINES);

		refresh();
		sleep(1);

		num++;
    }

    finish(0);
}

static void finish([[maybe_unused]] int sig) {
    endwin();

    exit(0);
}

