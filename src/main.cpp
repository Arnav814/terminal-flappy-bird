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
#include "colors.cpp"

using namespace std;

#define PIPE_GAP_VERT 24
#define PIPE_WIDTH 6
#define PIPE_GAP_HORIZ 80

#define GRAVITY 0.25
#define BIRD_X_POS round(COLS*2/4)
#define BIRD_JUMP_VELOCITY 2.0

#define PIPE_COLOR 1
#define BIRD_COLOR 2

#define BIRD_HEIGHT 2
#define BIRD_WIDTH 3
const SextantDrawing birdDrawing(
	{{2,0,0,2,2},
	 {2,2,2,2,2},
	 {0,2,2,2,0}}
);

const SextantDrawing gameOver(
	{{3,3,3,3,0,3,3,3,3,0,3,3,3,3,3,0,3,3,3,3,0,0,0,3,3,3,3,0,3,0,0,0,3,0,3,3,3,3,0,3,3,3,3},
	 {3,0,0,0,0,3,0,0,3,0,3,0,3,0,3,0,3,0,0,0,0,0,0,3,0,0,3,0,3,0,0,0,3,0,3,0,0,0,0,3,0,0,3},
	 {3,0,3,3,0,3,3,3,3,0,3,0,3,0,3,0,3,3,3,3,0,0,0,3,0,0,3,0,3,0,0,0,3,0,3,3,3,3,0,3,3,3,0},
	 {3,0,0,3,0,3,0,0,3,0,3,0,0,0,3,0,3,0,0,0,0,0,0,3,0,0,3,0,0,3,0,3,0,0,3,0,0,0,0,3,0,0,3},
	 {3,3,3,3,0,3,0,0,3,0,3,0,0,0,3,0,3,3,3,3,0,0,0,3,3,3,3,0,0,0,3,0,0,0,3,3,3,3,0,3,0,0,3}}
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
	assert(pipe.xPos >= 0 && pipe.xPos <= COLS*2);
	assertGt(pipe.height - PIPE_GAP_VERT/2, 0, "Invalid pipe height");
	assertGt(LINES*3 - (pipe.height + PIPE_GAP_VERT/2), 0, "Invalid pipe height");

	SextantDrawing topPipe(pipe.height - PIPE_GAP_VERT/2, PIPE_WIDTH+2);

	for (int y = 0; y < topPipe.getHeight(); y++) {
		assert(y >= 0 && y <= LINES*3);
		for (int x = 1; x < topPipe.getWidth()-1; x++) {
			topPipe.set(SextantCoord(y, x), PIPE_COLOR);
		}
	}

	topPipe.set(SextantCoord(topPipe.getHeight()-1, 0), PIPE_COLOR);
	topPipe.set(SextantCoord(topPipe.getHeight()-1, topPipe.getWidth()-1), PIPE_COLOR);

	SextantDrawing bottomPipe(LINES*3 - (pipe.height + PIPE_GAP_VERT/2), PIPE_WIDTH+2);

	for (int y = 0; y < bottomPipe.getHeight(); y++) {
		assert(y >= 0 && y <= LINES*3);
		for (int x = 1; x < bottomPipe.getWidth()-1; x++) {
			bottomPipe.set(SextantCoord(y, x), PIPE_COLOR);
		}
	}

	bottomPipe.set(SextantCoord(0, 0), PIPE_COLOR);
	bottomPipe.set(SextantCoord(0, bottomPipe.getWidth()-1), PIPE_COLOR);

	mvaddstr(logPos++, 15, to_string(pipe.xPos).c_str());
	//mvaddstr(16, 15, to_string(mainDrawing.getWidth()).c_str());

	mainDrawing.insert(SextantCoord(0, pipe.xPos - floor(PIPE_WIDTH/2)), topPipe);
	//topPipe.render(round((pipe.xPos - floor(PIPE_WIDTH/2) - 1) / 3), 0);
	mainDrawing.insert(SextantCoord(pipe.height + PIPE_GAP_VERT/2, pipe.xPos - floor(PIPE_WIDTH/2)), bottomPipe);
}

[[noreturn]] static void finish(int sig);

void displayRestartScr();

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
        init_pair(3, COLOR_RED, COLOR_BLACK);
    }

	vector<Pipe> pipes;
	short timeSinceLastPipe = PIPE_GAP_HORIZ+1;
	Bird bird(10.0, 0.0);

	SextantDrawing mainDrawing(LINES*3, COLS*2);

    while (true) {
		logPos = 15;

		assert(mainDrawing.getHeight() == LINES * 3);
		assert(mainDrawing.getWidth() == COLS * 2);

		char nextCh;
		while ((nextCh = getch()) != ERR) {
			switch (nextCh) {
				case ' ':
					bird.yVel = -BIRD_JUMP_VELOCITY;
					break;
				case 'p':
					sleep(10);
					break;
			}
		}

		erase();
		mainDrawing.clear();

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

		erase_if(pipes, [](const Pipe pipe) {return pipe.xPos <= floor(PIPE_WIDTH/2);});

		bird.yVel += GRAVITY;
		bird.yPos += bird.yVel;

		#define GAME_OVER do { \
				beep(); \
				displayRestartScr(); \
				bird.yPos = 10.0; \
				bird.yVel = 0.0; \
				pipes.clear(); \
				timeSinceLastPipe = 0; \
			} while (false)

		if (bird.yPos < 0)
			GAME_OVER;
		else if (bird.yPos + birdDrawing.getHeight() > LINES * 3)
			GAME_OVER;

		try {
			mainDrawing.insert(SextantCoord(bird.yPos, BIRD_X_POS), birdDrawing, OverrideStyle::Error);
		} catch(OverrideException const&) {
			GAME_OVER;
		}

		mvaddstr(5, 15, to_string(bird.yPos).c_str());
		mvaddstr(6, 15, to_string(bird.yVel).c_str());

		mainDrawing.render(CharCoord(0, 0));

		move(0, 0);
		refresh();
		this_thread::sleep_for(chrono::milliseconds(100));
    }

    finish(0);
}

void displayRestartScr() {
	CharCoord center = CharCoord(LINES/2, COLS/2);
	CharCoord topLeft =     center + CharCoord(-floor(gameOver.getHeight()/6)  , -floor(gameOver.getWidth()/4));
	//CharCoord topRight =    center + CharCoord(-floor(gameOver.getHeight()/6)  ,  floor(gameOver.getWidth()/4));
	CharCoord bottomLeft =  center + CharCoord(  ceil(gameOver.getHeight()/6)+2, - ceil(gameOver.getWidth()/4));
	CharCoord bottomRight = center + CharCoord(  ceil(gameOver.getHeight()/6)+2,   ceil(gameOver.getWidth()/4));

	gameOver.render(topLeft);
	mvaddstr(bottomLeft.y, bottomLeft.x, "CONTINUE?");
	string right = "QUIT?";
	mvaddstr(bottomRight.y, bottomRight.x - right.length() + 1, right.c_str());
	//mvaddch(LINES/2, COLS/2, 'O');
	refresh();
	nodelay(stdscr, false);
	char c;
	while (true) {
		c = getch();
		if (c == 'c' || c == 'C') {
			break;
		} else if (c == 'q' || 'Q') {
			finish(0);
		}
	}
	nodelay(stdscr, true);
}

[[noreturn]] static void finish([[maybe_unused]] int sig) {
    endwin();
    exit(0);
}

