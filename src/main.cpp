#include <cassert>
#include <clocale>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <thread>
#include <vector>
#include <limits>
#include <random>

#include "sextantBlocks.cpp"

using namespace std;

#define PIPE_GAP_VERT 6
#define PIPE_WIDTH 6
#define PIPE_GAP_HORIZ 40

#define GRAVITY 0.25
#define BIRD_X_POS round(LINES/4)
#define BIRD_JUMP_VELOCITY 2.0

#define PIPE_COLOR 1
#define BIRD_COLOR 2

#define BIRD_HEIGHT 2
#define BIRD_WIDTH 3
SextantDrawing BIRD_DRAWING(
	{{0,1,1,0},
	 {1,1,1,1},
	 {1,1,1,1},
	 {0,1,1,0}}
);

struct Pipe {double xPos; int height;};
struct Bird {double yPos; double yVel;};

// both min and max are inclusive
int randrange(int min, int max) {
    static default_random_engine engine {std::random_device{}()};
    std::uniform_int_distribution<int> uniform_dist(min, max);

    return uniform_dist(engine);
}

void drawPipe(const Pipe& pipe) {
	assert(pipe.xPos >= 0 && pipe.xPos <= COLS);
	attrset(COLOR_PAIR(PIPE_COLOR));

	for (int i = 0; i < pipe.height - PIPE_GAP_VERT/2; i++) {
		assert(i >= 0 && i <= LINES);
		for (int x = floor(-PIPE_WIDTH/2); x < ceil(PIPE_WIDTH/2); x++) {
			mvaddstr(i, round(pipe.xPos) + x, "█");
			//mvaddch(i, round(pipe.xPos) + x, x+65);
		}
	}

	for (int i = LINES; i > pipe.height + PIPE_GAP_VERT/2; i--) {
		assert(i >= 0 && i <= LINES);
		for (int x = floor(-PIPE_WIDTH/2); x < ceil(PIPE_WIDTH/2); x++) {
			mvaddstr(i, round(pipe.xPos) + x, "█");
			//mvaddch(i, round(pipe.xPos) + x, x+65);
		}
	}

	mvaddstr(pipe.height + PIPE_GAP_VERT/2 + 1, pipe.xPos - floor(PIPE_WIDTH/2) - 1, "█");
	mvaddstr(pipe.height - PIPE_GAP_VERT/2 - 1, pipe.xPos - floor(PIPE_WIDTH/2) - 1, "█");
	mvaddstr(pipe.height + PIPE_GAP_VERT/2 + 1, pipe.xPos + ceil(PIPE_WIDTH/2), "█");
	mvaddstr(pipe.height - PIPE_GAP_VERT/2 - 1, pipe.xPos + ceil(PIPE_WIDTH/2), "█");
}

static void finish(int sig);

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    signal(SIGINT, finish);      // arrange interrupts to terminate

	setlocale(LC_ALL, "");

    initscr(); // initialize the curses library
    cbreak(); // take input chars one at a time, no wait for \n
	noecho(); // don't echo output
	nodelay(stdscr, true); // don't wait for keypresses if there aren't any

    if (has_colors()) {
        start_color();

        init_pair(1, COLOR_GREEN, COLOR_BLACK);
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    }

	vector<Pipe> pipes;
	int timeSinceLastPipe = numeric_limits<int>::max();
	Bird bird(10.0, 0.0);

    while (true) {
		erase();

		switch (getch()) {
			case ' ':
				bird.yVel = -BIRD_JUMP_VELOCITY;
		}

		if (timeSinceLastPipe >= PIPE_GAP_HORIZ) {
			pipes.push_back(Pipe(COLS, randrange(PIPE_GAP_VERT / 2, LINES - PIPE_GAP_VERT / 2)));
			timeSinceLastPipe = 0;
		}
		timeSinceLastPipe++;

		for (Pipe& pipe: pipes) {
			drawPipe(pipe);
			pipe.xPos--;

			//cerr << pipe.xPos << ' ' << pipe.height << endl;
		}

		bird.yVel += GRAVITY;
		bird.yPos += bird.yVel;
		BIRD_DRAWING.render(BIRD_X_POS, bird.yPos);

		erase_if(pipes, [](const Pipe pipe) {return pipe.xPos < 0;});

		move(0, 0);
		refresh();
		this_thread::sleep_for(chrono::milliseconds(250));

		if (bird.yPos < 0)
			finish(0);
		else if (bird.yPos > LINES)
			finish(0);
    }

    finish(0);
}

static void finish([[maybe_unused]] int sig) {
    endwin();

    exit(0);
}

