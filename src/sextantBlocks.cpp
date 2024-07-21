#ifndef SEXTANTBLOCKS_CPP
#define SEXTANTBLOCKS_CPP
#include <cassert>
#include <cmath>
#include <unistd.h>
#include <vector>
#include <codecvt>
#include <locale>
#include <array>
#include <unordered_map>
#include <bitset>
#include <iostream>
#include <csignal>
#include <curses.h>

#include "types.cpp"
#include "colors.cpp"

using namespace std;

typedef unsigned char colorType;
typedef unsigned char priorityType;
template <typename storeAs> using charArray = array<array<storeAs, 3>, 2>;

struct PriorityColor {
	colorType color;
	priorityType priority;
};

char packArray(charArray<bool>& myArray) {
	return (
		((myArray[1][2] != 0 ? 1 : 0) << 0) +
		((myArray[1][1] != 0 ? 1 : 0) << 1) +
		((myArray[1][0] != 0 ? 1 : 0) << 2) +
		((myArray[0][2] != 0 ? 1 : 0) << 3) +
		((myArray[0][1] != 0 ? 1 : 0) << 4) +
		((myArray[0][0] != 0 ? 1 : 0) << 5)
	);
}

template<typename t> void print2DVector(const vector<vector<t>>& inputVec) {
	move(12, 10);
	int n = 0;
	for (const auto& x: inputVec) {
		for (const auto& y: x) {
			addstr(to_string(y).c_str());
		}
		move(12 + ++n, 10);
	}
	refresh();
	sleep(5);
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

void testAllSextants() {
	wstring_convert<codecvt_utf8<wchar_t>> utf8_conv;
	for (const auto& i: sextantMap) {
		bitset<6> asBitset(i.first);
		cout << (asBitset[5] ? '#' : '_');
		cout << (asBitset[2] ? '#' : '_') << '\n';
		cout << (asBitset[4] ? '#' : '_');
		cout << (asBitset[1] ? '#' : '_') << '\n';
		cout << (asBitset[3] ? '#' : '_');
		cout << (asBitset[0] ? '#' : '_') << '\n';
		cout << "\e[43m" << utf8_conv.to_bytes(i.second) << "\e[0m";
		cout << "\n\n";
	}
	cout << flush;
}

enum class OverrideStyle {Nonzero, Always, Priority, Error};
class OverrideException : public runtime_error {
	public:
		OverrideException(string error) : runtime_error(error.c_str()) {} ;
};

class RescaleException : public runtime_error {
	public:
		RescaleException(string error) : runtime_error(error.c_str()) {} ;
};

class SextantDrawing {
	private:
		vector<vector<PriorityColor>> drawing;
		[[nodiscard]] PriorityColor getWithFallback(const SextantCoord& coord, const PriorityColor fallback) const;
		[[nodiscard]] charArray<PriorityColor> getChar(const SextantCoord& topLeft) const;
	
	public:
		/*SextantDrawing(const vector<vector<colorType>>& setDrawing) {
			int height = setDrawing.size();
			int width = setDrawing[0].size();
			if (height == 0)
				throw RescaleException("Cannot create a SextantDrawing with height 0");
			this->drawing = vector<vector<PriorityColor>>(height, vector<PriorityColor>(width, PriorityColor(0, 0)));

			for (SextantCoord coord: this->getIterator()) {
				this->set(coord, PriorityColor(setDrawing[coord.y][coord.x], 0));
			}
		}*/
		SextantDrawing(const vector<vector<PriorityColor>>& setDrawing) {
			this->drawing = setDrawing;
		}
		SextantDrawing(const int height, const int width) {
			assertGt(height, 0, "height must be positive");
			assertGt(width, 0, "width must be positive");
			if (height == 0)
				throw RescaleException("Cannot create a SextantDrawing with height 0");
			this->drawing = vector<vector<PriorityColor>>(height, vector<PriorityColor>(width, PriorityColor(0, 0)));
		}
		[[nodiscard]] int getWidth() const {return this->drawing[0].size();}
		[[nodiscard]] int getHeight() const {return this->drawing.size();}
		[[nodiscard]] PriorityColor get(const SextantCoord& coord) const;
		[[nodiscard]] CoordIterator<SextantCoord> getIterator() const;
		void clear();
		void set(const SextantCoord& coord, const PriorityColor setTo);
		void trySet(const SextantCoord& coord, const PriorityColor setTo);
		void resize(int newY, int newX);
		void insert(const SextantCoord& topLeft, const SextantDrawing& toCopy, const OverrideStyle overrideStyle);
		void render(const CharCoord& topLeft) const;
		void debugPrint() const;
};

[[nodiscard]] PriorityColor SextantDrawing::get(const SextantCoord& coord) const {
	assertBetweenHalfOpen(0, coord.y, this->getHeight(), "Height out of range in get");
	assertBetweenHalfOpen(0, coord.x, this->getWidth(), "Width out of range in get");
	return this->drawing[coord.y][coord.x];
}

[[nodiscard]] PriorityColor SextantDrawing::getWithFallback(const SextantCoord& coord, const PriorityColor fallback) const {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth())
		return fallback;
	else [[likely]]
		return this->get(coord);
}

void SextantDrawing::clear() {
	for (int y = 0; y < this->getHeight(); y++) {
		for (int x = 0; x < this->getWidth(); x++) {
			this->drawing[y][x] = PriorityColor(0, 0);
		}
	}
}

void SextantDrawing::set(const SextantCoord& coord, const PriorityColor setTo) {
	assertBetweenHalfOpen(0, coord.y, (int) this->drawing.size(), "Height out of range in set");
	assertBetweenHalfOpen(0, coord.x, (int) this->drawing[coord.y].size(), "Width out of range in set");
	this->drawing[coord.y][coord.x] = setTo;
}

void SextantDrawing::trySet(const SextantCoord& coord, const PriorityColor setTo) {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth())
		return;
	else [[likely]]
		set(coord, setTo);
}

[[nodiscard]] CoordIterator<SextantCoord> SextantDrawing::getIterator() const {
	return CoordIterator(SextantCoord(0, 0), SextantCoord(this->getHeight()-1, this->getWidth()-1));
}

void SextantDrawing::resize(int newY, int newX) {
	if (newY == 0)
		throw RescaleException("Cannot resize a SextantDrawing to height 0");
	this->drawing.resize(newY);
	for (int y = 0; y < this->getHeight(); y++) {
		this->drawing[y].resize(newX);
	}
}

charArray<PriorityColor> SextantDrawing::getChar(const SextantCoord& topLeft) const {
	return {{
		{{
			getWithFallback(topLeft, PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(1, 0), PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(2, 0), PriorityColor(0, 0))
		}},
		{{
			getWithFallback(topLeft + SextantCoord(0, 1), PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(1, 1), PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(2, 1), PriorityColor(0, 0))
		}}
	}};
}

// copies toCopy onto this drawing
void SextantDrawing::insert(const SextantCoord& topLeft, const SextantDrawing& toCopy,
                            const OverrideStyle overrideStyle = OverrideStyle::Priority) {
	//cerr << topLeft.x << ' ' << topLeft.y << ';' << toCopy.getWidth() << ' ' << toCopy.getHeight() << endl;
	assertGtEq(topLeft.x, 0, "Top left must be greater than or equal to 0");
	assertGtEq(topLeft.y, 0, "Top left must be greater than or equal to 0");
	for (SextantCoord coord: CoordIterator(SextantCoord(0, 0),
			SextantCoord(min(this->getHeight() - topLeft.y - 1, toCopy.getHeight() - 1),
			             min(this->getWidth() - topLeft.x - 1, toCopy.getWidth() - 1)))) {
		bool doSet;

		switch (overrideStyle) {
			case OverrideStyle::Always:
				doSet = true;
				break;
			case OverrideStyle::Nonzero:
				doSet = this->get(topLeft + coord).priority == 0;
				break;
			case OverrideStyle::Priority:
				doSet = this->get(topLeft + coord).priority < toCopy.get(coord).priority;
				break;
			case OverrideStyle::Error:
				if (this->get(topLeft + coord).priority != 0) {
					throw OverrideException("Override attempted with OverrideStyle::Error set");
				} else {
					doSet = true;
				}
				break;
			default:
				assert(false); // makes GCC shut up
		}

		if (doSet)
			this->set(topLeft + coord, toCopy.get(coord));
	}
}

pair<colorType, colorType> getTrimmedColors(const charArray<PriorityColor>& arrayChar) {
	pair<priorityType, priorityType> priorities = make_pair(0, 0);
	pair<colorType, colorType> colors = make_pair(0, 0);

	for (auto a: arrayChar) {
		for (PriorityColor b: a) {
			// TODO: this doesn't handle PriorityColors with the
			// same color and different priorities very well.
			if (b.color != colors.first && b.color != colors.second) {
				if (b.priority > priorities.first) {
					colors.second = colors.first;
					priorities.second = priorities.first;

					colors.first = b.color;
					priorities.first = b.priority;
				} else if (b.priority > priorities.second) {
					colors.second = b.color;
					priorities.second = b.priority;
				}
			}
		}
	}

	if (colors.second == 0) {
		swap(colors.first, colors.second);
		// prefer setting bg over fg -- a space is 1/4 the bytes of a filled block
	}

	/*if ((colors.first == COLOR_YELLOW || colors.second == COLOR_YELLOW)
		&& (arrayChar[0][0].color != COLOR_YELLOW
		||  arrayChar[0][1].color != COLOR_YELLOW
		||  arrayChar[0][2].color != COLOR_YELLOW
		||  arrayChar[1][0].color != COLOR_YELLOW
		||  arrayChar[1][1].color != COLOR_YELLOW
		||  arrayChar[1][2].color != COLOR_YELLOW)) {
		raise(SIGINT);
	}*/

	return colors;
}

void SextantDrawing::render(const CharCoord& topLeft) const {
	//print2DVector(this->drawing);
	static wstring_convert<codecvt_utf8<wchar_t>> utf8_conv;
	for (int y = 0; y < this->getHeight(); y += 3) {
		for (int x = 0; x < this->getWidth(); x += 2) {
			charArray<PriorityColor> asArray = getChar(SextantCoord(y, x));
			pair<colorType, colorType> colors = getTrimmedColors(asArray);
			charArray<bool> trimmed;

			string myString = "";
			myString += to_string(colors.first);
			myString += " ";
			myString += to_string(colors.second);

			//attrset(getColorPair(COLOR_GREEN, COLOR_BLACK));
			//mvaddstr(10, 12, myString.c_str());

			for (unsigned int x = 0; x < asArray.size(); x++) {
				for (unsigned int y = 0; y < asArray[x].size(); y++) {
					if (asArray[x][y].color == colors.first) {
						trimmed[x][y] = true;
					} else {
						trimmed[x][y] = false;
					}
				}
			}

			attrset(getColorPair(colors.first, colors.second));
			mvaddstr(round(y/3) + topLeft.y, round(x/2) + topLeft.x,
					 utf8_conv.to_bytes(sextantMap[packArray(trimmed)]).c_str());
		}
	}
}

void SextantDrawing::debugPrint() const {
	for (int y = 0; y < this->getHeight(); y++) {
		for (int x = 0; x < this->getWidth(); x++) {
			cerr << (this->drawing[y][x].color ? "█" : " ");
		}
		cerr << '\n';
	}
	cerr << flush;
}

#endif /* SEXTANTBLOCKS_CPP */
