#include <cassert>
#include <chrono>
#include <clocale>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <thread>
#include <vector>
#include <random>

#include "types.cpp"
#include "colors.cpp"
#include "sextantBlocks.cpp"
#include "moreAssertions.cpp"

using namespace std;

// For things like jump height, etc, etc
namespace RuntimeConstants {
	uint pipeGapVert;
	uint pipeWidth;
	uint pipeGapHoriz;
	uint birdXPos;

	chrono::milliseconds frameDelay;
	uint pipeProcessFrames;
	// pipes will be processed every n frames
	// essentially makes the bird update more frequently than the pipes

	double gravity;
	double birdJumpVelocity;
};

#define PIPE_FILL PriorityColor(COLOR_GREEN, 100)
#define BIRD_COLOR COLOR_YELLOW

#define F PriorityColor(BIRD_COLOR, 101)
#define O PriorityColor(COLOR_BLACK, 0)
const SextantDrawing birdDrawing(
	{{F,O,O,F,F},
	 {F,F,F,F,F},
	 {O,F,F,F,O}}
);
#undef F
#undef O

#define F PriorityColor(COLOR_RED, 250)
#define O PriorityColor(COLOR_BLACK, 249)
const SextantDrawing gameOver(
	{{F,F,F,F,O,F,F,F,F,O,F,F,F,F,F,O,F,F,F,F,O,O,F,F,F,F,O,F,O,O,O,F,O,F,F,F,F,O,F,F,F,F},
	 {F,O,O,O,O,F,O,O,F,O,F,O,F,O,F,O,F,O,O,O,O,O,F,O,O,F,O,F,O,O,O,F,O,F,O,O,O,O,F,O,O,F},
	 {F,O,F,F,O,F,F,F,F,O,F,O,F,O,F,O,F,F,F,F,O,O,F,O,O,F,O,F,O,O,O,F,O,F,F,F,F,O,F,F,F,O},
	 {F,O,O,F,O,F,O,O,F,O,F,O,O,O,F,O,F,O,O,O,O,O,F,O,O,F,O,O,F,O,F,O,O,F,O,O,O,O,F,O,O,F},
	 {F,F,F,F,O,F,O,O,F,O,F,O,O,O,F,O,F,F,F,F,O,O,F,F,F,F,O,O,O,F,O,O,O,F,F,F,F,O,F,O,O,F},
	 {O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O}}
);
#undef F
#undef O

struct Pipe {int xPos; int height;};
struct Bird {double yPos; double yVel;};

//int logPos = 15;

// both min and max are inclusive
int randrange(int min, int max) {
    //static default_random_engine engine {random_device{}()};
    static default_random_engine engine {42}; // Makes debugging random segfaults less painful
    uniform_int_distribution<int> uniform_dist(min, max);

    return uniform_dist(engine);
}

void drawPipe(SextantDrawing& drawing, const Pipe& pipe) {
	assert(pipe.xPos >= 0 && pipe.xPos <= COLS*2);
	assertGt(pipe.height - RuntimeConstants::pipeGapVert/2, 0, "Invalid pipe height");
	assertGt(LINES*3 - (pipe.height + RuntimeConstants::pipeGapVert/2), 0, "Invalid pipe height");

	SextantDrawing topPipe(pipe.height - RuntimeConstants::pipeGapVert/2, RuntimeConstants::pipeWidth+2);

	for (int y = 0; y < topPipe.getHeight(); y++) {
		assert(y >= 0 && y <= LINES*3);
		for (int x = 1; x < topPipe.getWidth()-1; x++) {
			topPipe.set(SextantCoord(y, x), PIPE_FILL);
		}
	}

	// add the rims to the pipes
	topPipe.set(SextantCoord(topPipe.getHeight()-1, 0), PIPE_FILL);
	topPipe.set(SextantCoord(topPipe.getHeight()-1, topPipe.getWidth()-1), PIPE_FILL);

	SextantDrawing bottomPipe(LINES*3 - (pipe.height + RuntimeConstants::pipeGapVert/2), RuntimeConstants::pipeWidth+2);

	for (int y = 0; y < bottomPipe.getHeight(); y++) {
		assert(y >= 0 && y <= LINES*3);
		for (int x = 1; x < bottomPipe.getWidth()-1; x++) {
			bottomPipe.set(SextantCoord(y, x), PIPE_FILL);
		}
	}

	bottomPipe.set(SextantCoord(0, 0), PIPE_FILL);
	bottomPipe.set(SextantCoord(0, bottomPipe.getWidth()-1), PIPE_FILL);

	//mvaddstr(logPos++, 15, to_string(pipe.xPos).c_str());
	//mvaddstr(16, 15, to_string(mainDrawing.getWidth()).c_str());

	drawing.insert(SextantCoord(0, pipe.xPos - floor(RuntimeConstants::pipeWidth/2)), topPipe);
	//topPipe.render(round((pipe.xPos - floor(RuntimeConstants::pipeWidth/2) - 1) / 3), 0);
	drawing.insert(SextantCoord(pipe.height + RuntimeConstants::pipeGapVert/2, pipe.xPos - floor(RuntimeConstants::pipeWidth/2)), bottomPipe);
}

void drawBg(SextantDrawing& drawing) {
	for (SextantCoord coord: drawing.getIterator()) {
		drawing.set(coord, PriorityColor(COLOR_BLUE, 1));
	}
}

// Try to find sane values based on screen size if they aren't specified.
void guessConstants() {
	using namespace RuntimeConstants;

	pipeGapVert = birdDrawing.getHeight() * 8;
	pipeWidth = 6;
	pipeGapHoriz = round((double) COLS / 2);

	frameDelay = chrono::milliseconds(50);
	pipeProcessFrames = 2;

	gravity = 0.1;
	birdXPos = round((double) COLS * 2 / 4);
	birdJumpVelocity = gravity * 20;
}

[[noreturn]] static void finish(int sig);
void displayRestartScr(SextantDrawing& drawing);

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {

	#ifndef NDEBUG
	// Because I'm too lazy to setup proper tests.
	assertEq(gameOver.getWidth() % 2, 0, "Game over size must be a round number of chars.");
	assertEq(gameOver.getHeight() % 3, 0, "Game over size must be a round number of chars.");
	#endif

    signal(SIGINT, finish); // arrange interrupts to terminate

	setlocale(LC_ALL, "");

    initscr(); // initialize the curses library
    cbreak(); // take input chars one at a time, no wait for \n
	noecho(); // don't echo output
	nodelay(stdscr, true); // don't wait for keypresses if there aren't any

    if (has_colors()) {
        start_color();
    }

	guessConstants();

	vector<Pipe> pipes;
	uint timeSinceLastPipe = RuntimeConstants::pipeGapHoriz + 1;
	uint timeSincePipesProcessed = RuntimeConstants::pipeProcessFrames + 1;
	Bird bird(10.0, 0.0);

	SextantDrawing mainDrawing(LINES*3, COLS*2);
	SextantDrawing foregroundDrawing(LINES*3, COLS*2);

	bool isGameOver = false;

    while (true) {
		//logPos = 15;

		// TODO: better resize handling than crashing
		assertEq(mainDrawing.getHeight(), LINES * 3, "Main drawing sized incorrectly for terminal.");
		assertEq(mainDrawing.getWidth(), COLS * 2, "Main drawing sized incorrectly for terminal.");
		assertEq(foregroundDrawing.getHeight(), LINES * 3, "Foreground drawing sized incorrectly for terminal.");
		assertEq(foregroundDrawing.getWidth(), COLS * 2, "Foreground drawing sized incorrectly for terminal.");

		char nextCh;
		while ((nextCh = getch()) != ERR) {
			switch (nextCh) {
				case ' ':
					bird.yVel = -RuntimeConstants::birdJumpVelocity;
					break;
				case 'p':
					sleep(10);
					break;
			}
		}

		erase();
		mainDrawing.clear();
		foregroundDrawing.clear();

		drawBg(mainDrawing);

		for (Pipe& pipe: pipes) {
			drawPipe(foregroundDrawing, pipe);
		}

		if (timeSincePipesProcessed >= RuntimeConstants::pipeProcessFrames) {
			if (timeSinceLastPipe >= RuntimeConstants::pipeGapHoriz) {
				pipes.push_back(Pipe(foregroundDrawing.getWidth() - 1, randrange(RuntimeConstants::pipeGapVert / 2 + 1, LINES*3 - RuntimeConstants::pipeGapVert/2)));
				timeSinceLastPipe = 0;
			}
			timeSinceLastPipe++;

			for (Pipe& pipe: pipes) {
				pipe.xPos--;
			}

			// delete offscreen pipes
			erase_if(pipes, [](const Pipe pipe) {return pipe.xPos <= floor(RuntimeConstants::pipeWidth/2);});

			timeSincePipesProcessed = 0;
		}
		timeSincePipesProcessed++;

		bird.yVel += RuntimeConstants::gravity;
		bird.yPos += bird.yVel;

		if (bird.yPos < 0)
			isGameOver = true;
		else if (bird.yPos + birdDrawing.getHeight() > LINES * 3)
			isGameOver = true;

		if (!isGameOver) {
			try {
				foregroundDrawing.insert(SextantCoord(bird.yPos, RuntimeConstants::birdXPos), birdDrawing, OverrideStyle::Error);
			} catch(OverrideException&) {
				isGameOver = true;
			}
		}

		//mvaddstr(5, 15, to_string(bird.yPos).c_str());
		//mvaddstr(6, 15, to_string(bird.yVel).c_str());

		mainDrawing.insert(SextantCoord(0, 0), foregroundDrawing);

		if (isGameOver) {
				beep();
				displayRestartScr(mainDrawing);
				bird.yPos = 10.0;
				bird.yVel = 0.0;
				pipes.clear();
				timeSinceLastPipe = RuntimeConstants::pipeGapHoriz+1;
				isGameOver = false;
		}

		mainDrawing.render(CharCoord(0, 0));

		move(0, 0);
		refresh();
		this_thread::sleep_for(chrono::milliseconds(20));
    }

    finish(0);
}

// After writing this, I've concluded that layout managers are magic.
void displayRestartScr(SextantDrawing& drawing) {
	#define PADDING 1
	CharCoord center = CharCoord(LINES/2, COLS/2);

	uint height = gameOver.getHeight() / 3 + 3*PADDING;

	CharCoord topLeft = center - CharCoord(ceil((double) height / 2),
		ceil((double) gameOver.getWidth() / 4) + PADDING);
	CharCoord bottomRight = center + CharCoord(floor((double) height / 2),
		floor((double) gameOver.getWidth() / 4) + PADDING - 1 /* inexplicable -1 that makes it work */);

	// draw an outline
	for (SextantCoord coord: CoordIterator(SextantCoord(topLeft), SextantCoord(bottomRight) + SextantCoord(2, 1))) {
		drawing.set(coord, PriorityColor(COLOR_RED, 250));
	}

	// fill in black
	for (SextantCoord coord: CoordIterator(SextantCoord(topLeft) + SextantCoord(1, 1),
			SextantCoord(bottomRight) + SextantCoord(1, 0))) {
		drawing.set(coord, PriorityColor(COLOR_BLACK, 249));
	}

	drawing.insert(topLeft + CharCoord(PADDING, PADDING), gameOver);
	drawing.render(CharCoord(0, 0));

	attrset(getColorPair(COLOR_RED, COLOR_BLACK));
	mvaddstr(bottomRight.y - PADDING, topLeft.x + PADDING, "CONTINUE?");
	string right = "QUIT?";
	mvaddstr(bottomRight.y - PADDING,
		bottomRight.x - PADDING - right.length() + 1 /* inexplicable +1 */, right.c_str());
	//mvaddch(center.y, center.x, 'O');
	move(0, 0);
	#undef PADDING

	refresh();
	nodelay(stdscr, false);
	char c;
	while (true) {
		c = getch();
		if (c == 'c' || c == 'C') {
			break;
		} else if (c == 'q' || c == 'Q') {
			finish(0);
		}
	}
	nodelay(stdscr, true);
}

[[noreturn]] static void finish([[maybe_unused]] int sig) {
    endwin();
    exit(0);
}

