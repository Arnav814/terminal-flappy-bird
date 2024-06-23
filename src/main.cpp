#include <cassert>
#include <clocale>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <thread>
#include <vector>
#include <random>

#include "types.cpp"
#include "sextantBlocks.cpp"
#include "moreAssertions.cpp"

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
const SextantDrawing birdDrawing(
	{{0,2,2,0},
	 {2,2,2,2},
	 {2,2,2,2},
	 {0,2,2,0}}
);

struct Pipe {int xPos; int height;};
struct Bird {double yPos; double yVel;};

int logPos = 15;

// both min and max are inclusive
int randrange(int min, int max) {
    //static default_random_engine engine {random_device{}()};
    static default_random_engine engine {42}; // Makes debugging random segfaults less painful
    uniform_int_distribution<int> uniform_dist(min, max);

    return uniform_dist(engine);
}

void drawPipe(SextantDrawing& mainDrawing, const Pipe& pipe) {
	assert(pipe.xPos >= 0 && pipe.xPos <= COLS*3);
	assertGt(pipe.height - PIPE_GAP_VERT/2, 0, "Invalid pipe height");
	assertGt(LINES*3 - (pipe.height + PIPE_GAP_VERT/2), 0, "Invalid pipe height");

	SextantDrawing topPipe(pipe.height - PIPE_GAP_VERT/2, PIPE_WIDTH+2);

	for (int y = 0; y < topPipe.getHeight(); y++) {
		assert(y >= 0 && y <= LINES*3);
		for (int x = 1; x < topPipe.getWidth()-1; x++) {
			topPipe.trySet(SextantCoord(y, x), PIPE_COLOR);
		}
	}

	topPipe.trySet(SextantCoord(topPipe.getHeight(), 0), PIPE_COLOR);
	topPipe.trySet(SextantCoord(topPipe.getHeight(), topPipe.getWidth()), PIPE_COLOR);

	SextantDrawing bottomPipe(LINES*3 - (pipe.height + PIPE_GAP_VERT/2), PIPE_WIDTH+2);

	for (int y = 0; y < bottomPipe.getHeight(); y++) {
		assert(y >= 0 && y <= LINES*3);
		for (int x = 1; x < bottomPipe.getWidth()-1; x++) {
			bottomPipe.trySet(SextantCoord(y, x), PIPE_COLOR);
		}
	}

	bottomPipe.trySet(SextantCoord(bottomPipe.getHeight(), 0), PIPE_COLOR);
	bottomPipe.trySet(SextantCoord(bottomPipe.getHeight(), bottomPipe.getWidth()), PIPE_COLOR);

	mvaddstr(logPos++, 15, to_string(pipe.xPos).c_str());
	//mvaddstr(16, 15, to_string(mainDrawing.getWidth()).c_str());

	mainDrawing.insert(SextantCoord(0, pipe.xPos - floor(PIPE_WIDTH/2)), topPipe);
	//topPipe.render(round((pipe.xPos - floor(PIPE_WIDTH/2) - 1) / 3), 0);
	mainDrawing.insert(SextantCoord(pipe.height + PIPE_GAP_VERT/2, pipe.xPos - floor(PIPE_WIDTH/2)), bottomPipe);
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
	int timeSinceLastPipe = 0;
	Bird bird(10.0, 0.0);

	SextantDrawing mainDrawing(LINES*3, COLS*2);

    while (true) {
		logPos = 15;
		erase();
		mainDrawing.clear();

		assert(mainDrawing.getHeight() == LINES * 3);
		assert(mainDrawing.getWidth() == COLS * 2);

		char nextCh;
		while ((nextCh = getch()) != ERR) {
			switch (nextCh) {
				case ' ':
					bird.yVel = -BIRD_JUMP_VELOCITY;
			}
		}

		if (timeSinceLastPipe >= PIPE_GAP_HORIZ) {
			pipes.push_back(Pipe(mainDrawing.getWidth() - 1, randrange(PIPE_GAP_VERT / 2 + 1, LINES*3 - PIPE_GAP_VERT/2)));
			timeSinceLastPipe = 0;
		}
		timeSinceLastPipe++;

		for (Pipe& pipe: pipes) {
			drawPipe(mainDrawing, pipe);
			pipe.xPos--;

			//cerr << pipe.xPos << ' ' << pipe.height << endl;
		}

		bird.yVel += GRAVITY;
		bird.yPos += bird.yVel;

		if (bird.yPos < 0)
			finish(0);
		else if (bird.yPos + birdDrawing.getHeight() > LINES * 3)
			finish(0);

		mainDrawing.insert(SextantCoord(bird.yPos, BIRD_X_POS), birdDrawing);
		mvaddstr(5, 15, to_string(bird.yPos).c_str());
		mvaddstr(6, 15, to_string(bird.yVel).c_str());

		mainDrawing.render(CharCoord(0, 0));

		erase_if(pipes, [](const Pipe pipe) {return pipe.xPos < 0;});

		move(0, 0);
		refresh();
		this_thread::sleep_for(chrono::milliseconds(100));
    }

    finish(0);
}

static void finish([[maybe_unused]] int sig) {
    endwin();

    exit(0);
}

