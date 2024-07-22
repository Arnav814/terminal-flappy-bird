#include <cassert>
#include <chrono>
#include <clocale>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <thread>
#include <vector>
#include <random>
#include <memory>

#include "cxxopts/cxxopts.hpp"
#include "types.cpp"
#include "colors.cpp"
#include "sextantBlocks.cpp"
#include "backgroundGen.cpp"
#include "moreAssertions.cpp"

using namespace std;

// For things like jump height, etc, etc
namespace RuntimeConstants {
	bool showBackground;

	uint pipeGapVert;
	uint pipeWidth;
	uint pipeGapHoriz;
	uint birdXPos;
	uint pipeSpeed;
	uint pipeProcessFrames;
	// pipes will be processed every n frames
	// essentially makes the bird update more frequently than the pipes
	uint bgProcessFrames;
	// the same, but for the background

	chrono::milliseconds frameDelay;

	double gravity;
	double birdJumpVelocity;
};

short PIPE_FILL = 20;
short BIRD_COLOR = 21;
short HILL_COLOR = 22;
short SKY_COLOR = 23;
short MSG_COLOR = 24;
short MSG_BG = 25;

void initColors() {
	if (can_change_color()) {
		#define RGB(r, g, b) r*1000/256, g*1000/256, b*1000/256 // init_color wants 0-1000
		init_color(PIPE_FILL, RGB(34, 178, 58));
		init_color(BIRD_COLOR, RGB(228, 225, 74));
		init_color(HILL_COLOR, RGB(109, 205, 68));
		init_color(SKY_COLOR, RGB(75, 178, 223));
		init_color(MSG_COLOR, RGB(200, 0, 23));
		init_color(MSG_BG, RGB(25, 25, 25));
		#undef RGB
	} else {
		// use defualt colors if there is no RGB support
		// the bird has been made magenta to differentiate it from the hills
		// the hills have been made yellow to differentiate them from the pipes
		PIPE_FILL = COLOR_GREEN;
		BIRD_COLOR = COLOR_MAGENTA;
		HILL_COLOR = COLOR_YELLOW;
		SKY_COLOR = COLOR_BLUE;
		MSG_COLOR = COLOR_RED;
		MSG_BG = COLOR_BLACK;
	}
}

unique_ptr<SextantDrawing> birdDrawing;
unique_ptr<SextantDrawing> gameOver;
unique_ptr<SextantDrawing> paused;

void initDrawings() {
	#define F PriorityColor(BIRD_COLOR, 101)
	#define O PriorityColor(COLOR_BLACK, 0)
	birdDrawing = make_unique<SextantDrawing, vector<vector<PriorityColor>>> (
		{{F,O,O,F,F},
		 {F,F,F,F,F},
		 {O,F,F,F,O}}
	);
	#undef F
	#undef O

	#define F PriorityColor(MSG_COLOR, 250)
	#define O PriorityColor(MSG_BG, 249)
	gameOver = make_unique<SextantDrawing, vector<vector<PriorityColor>>> (
		{{F,F,F,F,O,F,F,F,F,O,F,F,F,F,F,O,F,F,F,F,O,O,F,F,F,F,O,F,O,O,O,F,O,F,F,F,F,O,F,F,F,F},
		 {F,O,O,O,O,F,O,O,F,O,F,O,F,O,F,O,F,O,O,O,O,O,F,O,O,F,O,F,O,O,O,F,O,F,O,O,O,O,F,O,O,F},
		 {F,O,F,F,O,F,F,F,F,O,F,O,F,O,F,O,F,F,F,F,O,O,F,O,O,F,O,F,O,O,O,F,O,F,F,F,F,O,F,F,F,O},
		 {F,O,O,F,O,F,O,O,F,O,F,O,O,O,F,O,F,O,O,O,O,O,F,O,O,F,O,O,F,O,F,O,O,F,O,O,O,O,F,O,O,F},
		 {F,F,F,F,O,F,O,O,F,O,F,O,O,O,F,O,F,F,F,F,O,O,F,F,F,F,O,O,O,F,O,O,O,F,F,F,F,O,F,O,O,F},
		 {O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O}}
		 // The extra line makes this an even 2 chars tall
	);
	#undef F
	#undef O

	#define F PriorityColor(MSG_COLOR, 250)
	#define O PriorityColor(MSG_BG, 249)
	paused = make_unique<SextantDrawing, vector<vector<PriorityColor>>> (
		{{F,F,F,F,O,F,F,F,F,O,F,O,O,F,O,F,F,F,F,O,F,F,F,F,O,F,F,F,O},
		 {F,O,O,F,O,F,O,O,F,O,F,O,O,F,O,F,O,O,O,O,F,O,O,O,O,F,O,O,F},
		 {F,F,F,F,O,F,F,F,F,O,F,O,O,F,O,F,F,F,F,O,F,F,F,F,O,F,O,O,F},
		 {F,O,O,O,O,F,O,O,F,O,F,O,O,F,O,O,O,O,F,O,F,O,O,O,O,F,O,O,F},
		 {F,O,O,O,O,F,O,O,F,O,F,F,F,F,O,F,F,F,F,O,F,F,F,F,O,F,F,F,O},
		 {O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O}}
	);
	#undef F
	#undef O
}

struct Pipe {
	int xPos; int height; bool isPassed;
	Pipe(int xPos, int height) {
		this->xPos = xPos;
		this->height = height;
		this->isPassed = false;
	}
};

struct Bird {double yPos; double yVel;};

// both min and max are inclusive
int randrange(int min, int max) {
    static default_random_engine engine {random_device{}()};
    //static default_random_engine engine {42}; // Makes debugging random segfaults less painful
    uniform_int_distribution<int> uniform_dist(min, max);

    return uniform_dist(engine);
}

void drawPipe(SextantDrawing& drawing, const Pipe& pipe) {
	assert(pipe.xPos >= 0 && pipe.xPos <= COLS*2);
	assertGt(pipe.height - (int) RuntimeConstants::pipeGapVert/2, 0, "Invalid pipe height");
	assertLt(pipe.height + (int) RuntimeConstants::pipeGapVert/2, LINES * 3, "Invalid pipe height");

	SextantDrawing topPipe(pipe.height - RuntimeConstants::pipeGapVert/2, RuntimeConstants::pipeWidth+2);

	for (int y = 0; y < topPipe.getHeight(); y++) {
		assertGtEq(y, 0, "");
		assertLtEq(y, LINES * 3, "");
		for (int x = 1; x < topPipe.getWidth()-1; x++) {
			topPipe.set(SextantCoord(y, x), PriorityColor(PIPE_FILL, 100));
		}
	}

	// add the rims to the pipes
	topPipe.set(SextantCoord(topPipe.getHeight()-1, 0), PriorityColor(PIPE_FILL, 100));
	topPipe.set(SextantCoord(topPipe.getHeight()-1, topPipe.getWidth()-1), PriorityColor(PIPE_FILL, 100));

	SextantDrawing bottomPipe(LINES*3 - (pipe.height + RuntimeConstants::pipeGapVert/2), RuntimeConstants::pipeWidth+2);

	for (int y = 0; y < bottomPipe.getHeight(); y++) {
		assertGtEq(y, 0, "");
		assertLtEq(y, LINES * 3, "");
		for (int x = 1; x < bottomPipe.getWidth()-1; x++) {
			bottomPipe.set(SextantCoord(y, x), PriorityColor(PIPE_FILL, 100));
		}
	}

	bottomPipe.set(SextantCoord(0, 0), PriorityColor(PIPE_FILL, 100));
	bottomPipe.set(SextantCoord(0, bottomPipe.getWidth()-1), PriorityColor(PIPE_FILL, 100));

	drawing.insert(SextantCoord(0, pipe.xPos - floor(RuntimeConstants::pipeWidth/2)), topPipe);
	//topPipe.render(round((pipe.xPos - floor(RuntimeConstants::pipeWidth/2) - 1) / 3), 0);
	drawing.insert(SextantCoord(pipe.height + RuntimeConstants::pipeGapVert/2, pipe.xPos - floor(RuntimeConstants::pipeWidth/2)), bottomPipe);
}

uint currentBgPos = 0; // It was easiest to make this a global, so yeah

void drawBg(SextantDrawing& drawing) {
	static vector<uint> cache = {0};

	cache.erase(cache.begin());
	while (cache.size() < (uint) COLS * 2) {
		cache.push_back(getBgVal(currentBgPos++));
	}

	for (SextantCoord coord: drawing.getIterator()) {
		if (LINES * 3 - cache[coord.x] < (uint) coord.y) {
			drawing.set(coord, PriorityColor(HILL_COLOR, 2));
		} else {
			drawing.set(coord, PriorityColor(SKY_COLOR, 1));
		}
	}
}

[[noreturn]] void finish(int sig);
[[noreturn]] void exitStatus(int status);

// Get constants from CLI args.
// Call before curses init
[[nodiscard]] cxxopts::ParseResult parseArgs(int argc, char* argv[]) {
	cxxopts::Options options("flappy-bird", "A terminal Flappy Bird game.");

	// TODO: document units
	options.add_options()
		("v,vert-gap", "Set the gap of the hole in pipes", cxxopts::value<uint>())
		("h,horiz-gap", "Set the distance between pipes", cxxopts::value<uint>())
		("w,pipe-width", "Set the width of pipes", cxxopts::value<uint>())
		("s,pipe-speed", "Sets how fast the pipes move", cxxopts::value<uint>())

		("g,gravity", "Sets the strength of gravity", cxxopts::value<double>())
		("j,jump-speed", "Sets how much force jumps create", cxxopts::value<double>())
		("x,bird-x", "Sets the left-right position of the bird", cxxopts::value<uint>())

		("b,no-background", "Disables the background")
		("B,background", "Enables the background (this is the default)")

		("f,frame-rate", "Sets the framerate to update the bird", cxxopts::value<uint>())
		("p,pipe-skip-frames", "Renders pipes only every n frames to make the bird smoother", cxxopts::value<uint>())
		("S,bg-skip-frames", "Renders pipes only every n frames to make things smoother", cxxopts::value<uint>())

		("help", "Print usage")
		;

	class cliParseException : public runtime_error {
		public:
			cliParseException(const char* reason) : runtime_error(reason) {};
	};

	cxxopts::ParseResult parsed;
	try {
		parsed = options.parse(argc, argv);

		// do any other input validation here
		if (parsed.count("frame-rate") && parsed["frame-rate"].as<uint>() == 0)
			throw cliParseException("frame rate cannot be 0");
		if (parsed.count("pipe-speed") && parsed["pipe-speed"].as<uint>() == 0)
			throw cliParseException("pipe speed cannot be 0");
	} catch (const cxxopts::exceptions::parsing& e) {
		cerr << "Error while parsing command line arguments: " << e.what() << endl;
		exit(1);
	} catch (const cliParseException& e) {
		cerr << "Invalid command line argument: " << e.what() << endl;
		exit(1);
	}

	if (parsed.count("help")) {
		cout << options.help() << endl;
		exit(0);
	}

	return parsed;
}

// Apply the parsed args
// Try to guess sane defaults based on screen size if they aren't specified
// Call after curses init
void setArgs(cxxopts::ParseResult& parsed) {
	using namespace RuntimeConstants;

	#define setWithDefault(varName, paramName, fallback) \
		varName = parsed.count(paramName) ? parsed[paramName].as<decltype(varName)>() : (fallback)

	showBackground = true;
	if (parsed.count("no-background"))
		showBackground = false;
	if (parsed.count("background"))
		showBackground = true;
	// the default is true, and background takes precedence over no-background

	setWithDefault(pipeGapVert, "vert-gap", birdDrawing->getHeight() * 8);
	setWithDefault(pipeWidth, "pipe-width", 6);
	setWithDefault(pipeSpeed, "pipe-speed", 1);
	setWithDefault(pipeGapHoriz, "horiz-gap", round(((double) COLS / 4 + 10) / pipeSpeed));
	setWithDefault(pipeProcessFrames, "pipe-skip-frames", 1);
	setWithDefault(bgProcessFrames, "bg-skip-frames", 1);

	// this one is a bit more complicated -- we need to take the reciprocal and convert to milliseconds
	frameDelay = chrono::milliseconds((uint) round(1.0 / (parsed.count("frame-rate") ? parsed["frame-rate"].as<uint>() : 10) * 1000));

	setWithDefault(gravity, "gravity", 0.1);
	setWithDefault(birdXPos, "bird-x", round((double) COLS * 2 / 4));
	setWithDefault(birdJumpVelocity, "jump-speed", gravity * 20);

	#undef setWithDefault
}

void displayMsgAndPause(SextantDrawing& drawing, const SextantDrawing& message, const uint score);

bool screenResized = false;
void setScreenResized([[maybe_unused]] int sig) {
	screenResized = true;
}

int main(int argc, char* argv[]) {
	auto parsed = parseArgs(argc, argv);

    signal(SIGINT, finish); // arrange interrupts to terminate
	signal(SIGTERM, finish);
	signal(SIGQUIT, finish);
	signal(SIGSEGV, finish);

	signal(SIGWINCH, setScreenResized);

	setlocale(LC_ALL, "");

    initscr(); // initialize the curses library
    cbreak(); // take input chars one at a time, no wait for \n
	noecho(); // don't echo output
	nodelay(stdscr, true); // don't wait for keypresses if there aren't any

    if (has_colors()) {
        start_color();
    } else {
		cerr << "This program requires a terminal with color support. Check you $TERM environment variable." << endl;
		exitStatus(4);
	}

	initColors();
	initDrawings();

	setArgs(parsed);
	if (RuntimeConstants::showBackground)
		initSines();


	vector<Pipe> pipes;
	uint timeSinceLastPipe = RuntimeConstants::pipeGapHoriz + 1;
	uint timeSincePipesProcessed = RuntimeConstants::pipeProcessFrames + 1;
	Bird bird(10.0, 0.0);

	SextantDrawing finalDrawing(LINES*3, COLS*2);
	SextantDrawing bgDrawing(LINES*3, COLS*2);
	SextantDrawing foregroundDrawing(LINES*3, COLS*2);

	uint timeSinceBgProcessed = RuntimeConstants::bgProcessFrames + 1;

	uint score = 0;

	bool isGameOver = false;
	auto startTs = chrono::system_clock::now(); // the set is so I don't have to type the type

    while (true) {
		//logPos = 15;
		startTs = chrono::system_clock::now();

		if (screenResized) {
			// pause so things only need to be rescaled once
			displayMsgAndPause(finalDrawing, *paused, score);
			screenResized = false;
			
			// actually resize the screen
			CharCoord oldSize(LINES, COLS);
			winsize newWindowSize;
			int ioctlStatus = ioctl(STDIN_FILENO, TIOCGWINSZ, &newWindowSize);
			if (ioctlStatus)
				throw runtime_error("ioctl is doing bad stuff. Status: " +  to_string(ioctlStatus)
					+ ". I have no clue how/why it works or why it isn't working now. Sorry. :-[");
			resizeterm(newWindowSize.ws_row, newWindowSize.ws_col);

			// resize drawings
			finalDrawing.resize(LINES * 3, COLS * 2);
			bgDrawing.resize(LINES * 3, COLS * 2);
			foregroundDrawing.resize(LINES * 3, COLS * 2);

			// make sure everything is redrawn
			timeSincePipesProcessed = RuntimeConstants::pipeProcessFrames + 1;
			timeSinceBgProcessed = RuntimeConstants::bgProcessFrames + 1;

			double rescaleCoefX [[maybe_unused]] = (double) COLS / oldSize.x;
			double rescaleCoefY = (double) LINES / oldSize.y;

			// rescale everything so it fits properly
			bird.yPos *= rescaleCoefY;
			erase_if(pipes, [](const Pipe pipe){return pipe.xPos > COLS * 2;});
			for (Pipe& pipe: pipes) {
				pipe.height *= rescaleCoefY;
			}

			clear();
		}

		assertEq(finalDrawing.getHeight(), LINES * 3, "Final drawing sized incorrectly for terminal.");
		assertEq(finalDrawing.getWidth(), COLS * 2, "Final drawing sized incorrectly for terminal.");
		assertEq(bgDrawing.getHeight(), LINES * 3, "Background drawing sized incorrectly for terminal.");
		assertEq(bgDrawing.getWidth(), COLS * 2, "Background drawing sized incorrectly for terminal.");
		assertEq(foregroundDrawing.getHeight(), LINES * 3, "Foreground drawing sized incorrectly for terminal.");
		assertEq(foregroundDrawing.getWidth(), COLS * 2, "Foreground drawing sized incorrectly for terminal.");


		char nextCh;
		while ((nextCh = getch()) != ERR) {
			switch (nextCh) {
				case ' ':
					bird.yVel = -RuntimeConstants::birdJumpVelocity;
					break;
				case 'p':
				case 'P':
				case '\e':
					displayMsgAndPause(finalDrawing, *paused, score);
					break;
			}
		}

		erase();
		finalDrawing.clear();
		foregroundDrawing.clear();

		if (RuntimeConstants::showBackground) {
			if (timeSinceBgProcessed >= RuntimeConstants::bgProcessFrames) {
				bgDrawing.clear();
				drawBg(bgDrawing);
				timeSinceBgProcessed = 0;
			}
			timeSinceBgProcessed++;
		}

		for (Pipe& pipe: pipes) {
			if (pipe.xPos + RuntimeConstants::pipeWidth/2 + 1 < // +1 to account for the pipe rims
					RuntimeConstants::birdXPos - birdDrawing->getWidth()/2 &&
					!pipe.isPassed) {
				score++;
				pipe.isPassed = true;
			}
			drawPipe(foregroundDrawing, pipe);
		}

		if (timeSincePipesProcessed >= RuntimeConstants::pipeProcessFrames) {
			if (timeSinceLastPipe >= RuntimeConstants::pipeGapHoriz) {
				pipes.push_back(Pipe(foregroundDrawing.getWidth() - 1,
					randrange(RuntimeConstants::pipeGapVert / 2 + 1, LINES*3 - RuntimeConstants::pipeGapVert/2 - 1)));
				timeSinceLastPipe = 0;
			}
			timeSinceLastPipe++;

			for (Pipe& pipe: pipes) {
				pipe.xPos -= RuntimeConstants::pipeSpeed;
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
		else if (bird.yPos + birdDrawing->getHeight() > LINES * 3)
			isGameOver = true;

		if (!isGameOver) {
			try {
				foregroundDrawing.insert(SextantCoord(bird.yPos, RuntimeConstants::birdXPos), *birdDrawing, OverrideStyle::Error);
			} catch(OverrideException&) {
				isGameOver = true;
			}
		}

		//mvaddstr(5, 15, to_string(bird.yPos).c_str());
		//mvaddstr(6, 15, to_string(bird.yVel).c_str());

		if (RuntimeConstants::showBackground)
			finalDrawing.insert(SextantCoord(0, 0), bgDrawing);

		finalDrawing.insert(SextantCoord(0, 0), foregroundDrawing);

		if (isGameOver) {
				beep();
				displayMsgAndPause(finalDrawing, *gameOver, score);
				bird.yPos = 10.0;
				bird.yVel = 0.0;
				pipes.clear();
				timeSinceLastPipe = RuntimeConstants::pipeGapHoriz+1;
				if (RuntimeConstants::showBackground) {
					timeSinceBgProcessed = RuntimeConstants::bgProcessFrames+1;
					initSines();
				}
				score = 0;
				isGameOver = false;
		}

		finalDrawing.render(CharCoord(0, 0));

		move(0, 0);
		refresh();
		this_thread::sleep_for(RuntimeConstants::frameDelay - (chrono::system_clock::now() - startTs));
    }

    exitStatus(2); // should be unreachable
}

// After writing this, I've concluded that layout managers are magic.
void displayMsgAndPause(SextantDrawing& drawing, const SextantDrawing& message, const uint score) {
	#define PADDING 1
	CharCoord center = CharCoord(LINES/2, COLS/2);

	uint height = message.getHeight() / 3 + 3*PADDING;

	CharCoord topLeft = center - CharCoord(ceil((double) height / 2),
		ceil((double) message.getWidth() / 4) + PADDING);
	CharCoord bottomRight = center + CharCoord(floor((double) height / 2),
		floor((double) message.getWidth() / 4) + PADDING - 1 /* inexplicable -1 that makes it work */);

	// draw an outline
	for (SextantCoord coord: CoordIterator(SextantCoord(topLeft), SextantCoord(bottomRight) + SextantCoord(2, 1))) {
		drawing.set(coord, PriorityColor(MSG_COLOR, 250));
	}

	// fill in black
	for (SextantCoord coord: CoordIterator(SextantCoord(topLeft) + SextantCoord(1, 1),
			SextantCoord(bottomRight) + SextantCoord(1, 0))) {
		drawing.set(coord, PriorityColor(MSG_BG, 249));
	}

	drawing.insert(topLeft + CharCoord(PADDING, PADDING), message);
	drawing.render(CharCoord(0, 0));

	attrset(getColorPair(MSG_COLOR, MSG_BG));
	string scoreStr = "SCORE: " + to_string(score);
	mvaddstr(bottomRight.y - PADDING - 1, topLeft.x + PADDING, scoreStr.c_str());
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
			exitStatus(0);
		}
	}
	nodelay(stdscr, true);
}

[[noreturn]] void finish(int sig) {
	switch (sig) {
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			exitStatus(0);
			break;
		case SIGSEGV:
			exitStatus(3);
			break;
		default:
			exitStatus(4);
			break;
	}
}

[[noreturn]] void exitStatus(int status) {
	endwin();
	exit(status);
}

