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
#include <curses.h>

#include "types.cpp"
#include "colors.cpp"

using namespace std;

typedef short colorType;
static_assert(numeric_limits<colorType>::min() < 0, "colorType must be capable of storing negative numbers");

typedef array<array<colorType, 3>, 2> charArray;

char packArray(charArray& myArray) {
	return (
		((myArray[1][2] > 0 ? 1 : 0) << 0) +
		((myArray[1][1] > 0 ? 1 : 0) << 1) +
		((myArray[1][0] > 0 ? 1 : 0) << 2) +
		((myArray[0][2] > 0 ? 1 : 0) << 3) +
		((myArray[0][1] > 0 ? 1 : 0) << 4) +
		((myArray[0][0] > 0 ? 1 : 0) << 5)
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
		vector<vector<colorType>> drawing;
		[[nodiscard]] colorType getWithFallback(const SextantCoord& coord, const colorType fallback) const;
		[[nodiscard]] charArray getChar(const SextantCoord& topLeft) const;
	
	public:
		SextantDrawing(const vector<vector<colorType>>& setDrawing) {
			this->drawing = setDrawing;
		}
		SextantDrawing(const int height, const int width) {
			if (height == 0)
				throw RescaleException("Cannot create a SextantDrawing with height 0");
			this->drawing = vector<vector<colorType>>(height, vector<colorType>(width, 0));
		}
		[[nodiscard]] int getWidth() const {return this->drawing[0].size();}
		[[nodiscard]] int getHeight() const {return this->drawing.size();}
		[[nodiscard]] colorType get(const SextantCoord& coord) const;
		void clear();
		void set(const SextantCoord& coord, const colorType setTo);
		void trySet(const SextantCoord& coord, const colorType setTo);
		void resize(int newY, int newX);
		void insert(const SextantCoord& topLeft, const SextantDrawing& toCopy, const OverrideStyle overrideStyle);
		void render(const CharCoord& topLeft) const;
		void debugPrint() const;
};

[[nodiscard]] colorType SextantDrawing::get(const SextantCoord& coord) const {
	assertBetweenHalfOpen(0, coord.y, this->getHeight(), "Height out of range in get");
	assertBetweenHalfOpen(0, coord.x, this->getWidth(), "Width out of range in get");
	return this->drawing[coord.y][coord.x];
}

colorType SextantDrawing::getWithFallback(const SextantCoord& coord, const colorType fallback) const {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth())
		return fallback;
	else [[likely]]
		return drawing[coord.y][coord.x];
}

void SextantDrawing::clear() {
	for (int y = 0; y < this->getHeight(); y++) {
		for (int x = 0; x < this->getWidth(); x++) {
			this->drawing[y][x] = 0;
		}
	}
}

void SextantDrawing::set(const SextantCoord& coord, const colorType setTo) {
	assertBetweenHalfOpen(0, coord.y, (int) this->drawing.size(), "Height out of range in set");
	assertBetweenHalfOpen(0, coord.x, (int) this->drawing[coord.y].size(), "Width out of range in set");
	this->drawing[coord.y][coord.x] = setTo;
}

void SextantDrawing::trySet(const SextantCoord& coord, const colorType setTo) {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth())
		return;
	else [[likely]]
		set(coord, setTo);
}

void SextantDrawing::resize(int newY, int newX) {
	if (newY == 0)
		throw RescaleException("Cannot resize a SextantDrawing to height 0");
	this->drawing.resize(newY);
	for (int y = 0; y < this->getHeight(); y++) {
		this->drawing[y].resize(newX);
	}
}

charArray SextantDrawing::getChar(const SextantCoord& topLeft) const {
	return {{
		{{
			getWithFallback(topLeft, 0),
			getWithFallback(topLeft + SextantCoord(1, 0), 0),
			getWithFallback(topLeft + SextantCoord(2, 0), 0)
		}},
		{{
			getWithFallback(topLeft + SextantCoord(0, 1), 0),
			getWithFallback(topLeft + SextantCoord(1, 1), 0),
			getWithFallback(topLeft + SextantCoord(2, 1), 0)
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
		//if (this->getWithFallback(y+topLeftY, x+topLeftX, 0) == 0)
		bool doSet;

		switch (overrideStyle) {
			case OverrideStyle::Always:
				doSet = true;
				break;
			case OverrideStyle::Nonzero:
				doSet = this->getWithFallback(topLeft + coord, 0) == 0;
				break;
			case OverrideStyle::Priority:
				doSet = this->getWithFallback(topLeft + coord, 0) < toCopy.get(coord);
				break;
			case OverrideStyle::Error:
				if (this->getWithFallback(topLeft + coord, 0) != 0) {
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

void trimColors(charArray& arrayChar) {
	pair<colorType, colorType> fgColors = make_pair(-1, -1);
	pair<colorType, colorType> bgColors = make_pair(1, 1);
	for (auto a: arrayChar) {
		for (colorType b: a) {
			if (b >= 0) {
				fgColors.first = max(fgColors.first, b);
				// .second should be the second highest
				if (b < fgColors.first)
					fgColors.second = max(fgColors.second, b);
			}
			if (b <= 0) {
				bgColors.first = max(bgColors.first, b);
				if (b < bgColors.first)
					bgColors.second = max(bgColors.second, b);
			}
		}
	}

	pair<colorType, colorType> clippedColors;
	if (fgColors.first < 0) {
		clippedColors = make_pair(
			bgColors.first > 0 ? 0 : -bgColors.first,
			bgColors.second > 0 ? -1 : -bgColors.second);
	} else if (bgColors.first > 0) {
		clippedColors = make_pair(
			fgColors.first < 0 ? 0 : fgColors.first,
			fgColors.second < 0 ? -1 : fgColors.second);
	} else {
		clippedColors = make_pair(
			fgColors.first,
			-bgColors.first);
	}

	for (unsigned int x = 0; x < arrayChar.size(); x++) {
		for (unsigned int y = 0; y < arrayChar[x].size(); y++) {
			if (arrayChar[x][y] == clippedColors.first) {
				;
			} else {
				arrayChar[x][y] = clippedColors.second;
			}
		}
	}
}

void SextantDrawing::render(const CharCoord& topLeft) const {
	//print2DVector(this->drawing);
	static wstring_convert<codecvt_utf8<wchar_t>> utf8_conv;
	for (int y = 0; y < this->getHeight(); y += 3) {
		for (int x = 0; x < this->getWidth(); x += 2) {
			charArray asArray = getChar(SextantCoord(y, x));
			trimColors(asArray);

			colorType fgVal = 0;
			colorType bgVal = 0;

			for (auto a: asArray) {
				for (colorType b: a) {
					fgVal = max(fgVal, b);
					if (b < 0)
						bgVal = max(bgVal, b);
				}
			}

			if (fgVal != 0 || bgVal != 0) {
				attrset(getColorPair(fgVal, bgVal));
				mvaddstr(round(y/3) + topLeft.y, round(x/2) + topLeft.x,
				         utf8_conv.to_bytes(sextantMap[packArray(asArray)]).c_str());
			}

			/*string myString = "";
			myString += to_string(asArray[0][0]);
			myString += " ";
			myString += to_string(asArray[0][1]);
			myString += " ";
			myString += to_string(asArray[0][2]);
			myString += " ";
			myString += to_string(asArray[1][0]);
			myString += " ";
			myString += to_string(asArray[1][1]);
			myString += " ";
			myString += to_string(asArray[1][2]);
			myString += "--";
			myString += to_string(packArray(asArray));
			mvaddstr(20, 10, myString.c_str());
			refresh();
			sleep(5);*/
		}
	}
}

void SextantDrawing::debugPrint() const {
	for (int y = 0; y < this->getHeight(); y++) {
		for (int x = 0; x < this->getWidth(); x++) {
			cout << (this->drawing[y][x] ? "█" : " ");
		}
		cout << '\n';
	}
	cout << flush;
}

#endif /* SEXTANTBLOCKS_CPP */
