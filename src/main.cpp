#include <cassert>
#include <clocale>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <thread>
#include <unordered_map>
#include <vector>
#include <limits>
#include <random>

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
const array<char[BIRD_WIDTH*4 + 1], BIRD_HEIGHT> BIRD_TEXT = {"🬭🬞█", "🬊█🬄"};

char packBoolArray(array<array<bool, 3>, 2>& boolArray) {
	return (
		(boolArray[0][0] >> 0) +
		(boolArray[0][1] >> 1) +
		(boolArray[0][2] >> 2) +
		(boolArray[1][0] >> 3) +
		(boolArray[1][1] >> 4) +
		(boolArray[1][2] >> 5)
	);
}

// Numbered:
// 0 3
// 1 4
// 2 5

unordered_map<char, wchar_t> sextantMap {
	{0b000000, L'⠀'},
	{0b000001, L'🬞'},
	{0b000010, L'🬇'},
	{0b000011, L'🬦'},
	{0b000100, L'🬁'},
	{0b000101, L'🬠'},
	{0b000110, L'🬉'},
	{0b000111, L'▐'},
	{0b001000, L'🬏'},
	{0b001001, L'🬭'},
	{0b001010, L'🬖'},
	{0b001011, L'🬵'},
	{0b001100, L'🬑'},
	{0b001101, L'🬯'},
	{0b001110, L'🬘'},
	{0b001111, L'🬷'},
	{0b010000, L'🬃'},
	{0b010001, L'🬢'},
	{0b010010, L'🬋'},
	{0b010011, L'🬩'},
	{0b010100, L'🬅'},
	{0b010101, L'🬤'},
	{0b010110, L'🬍'},
	{0b010111, L'🬫'},
	{0b011000, L'🬓'},
	{0b011001, L'🬱'},
	{0b011010, L'🬚'},
	{0b011011, L'🬹'},
	{0b011100, L'🬔'},
	{0b011101, L'🬳'},
	{0b011110, L'🬜'},
	{0b011111, L'🬻'},
	{0b100000, L'🬀'},
	{0b100001, L'🬟'},
	{0b100010, L'🬈'},
	{0b100011, L'🬧'},
	{0b100100, L'🬂'},
	{0b100101, L'🬡'},
	{0b100110, L'🬊'},
	{0b100111, L'🬨'},
	{0b101000, L'🬐'},
	{0b101001, L'🬮'},
	{0b101010, L'🬗'},
	{0b101011, L'🬶'},
	{0b101100, L'🬒'},
	{0b101101, L'🬰'},
	{0b101110, L'🬙'},
	{0b101111, L'🬸'},
	{0b110000, L'🬄'},
	{0b110001, L'🬣'},
	{0b110010, L'🬌'},
	{0b110011, L'🬪'},
	{0b110100, L'🬆'},
	{0b110101, L'🬥'},
	{0b110110, L'🬎'},
	{0b110111, L'🬬'},
	{0b111000, L'▌'},
	{0b111001, L'🬲'},
	{0b111010, L'🬛'},
	{0b111011, L'🬺'},
	{0b111100, L'🬕'},
	{0b111101, L'🬴'},
	{0b111110, L'🬝'},
	{0b111111, L'█'}
};

class SextantDrawing {
	private:
		vector<vector<bool>> drawing;
	
	public:
		SextantDrawing(const vector<vector<bool>>& setDrawing) {
			this->drawing = setDrawing;
		}
		void render(double topLeftX, double topLeftY) const
};

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

void drawBird(const Bird& bird) {
	for (int y = 0; y < BIRD_HEIGHT; y++) {
		mvaddstr(round(bird.yPos) + y, BIRD_X_POS, BIRD_TEXT[y]);
	}
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
		drawBird(bird);

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

