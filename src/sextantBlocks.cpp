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

using namespace std;

char packArray(array<array<unsigned char, 3>, 2>& myArray) {
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
		vector<vector<unsigned char>> drawing;
		[[nodiscard]] unsigned char getWithFallback(int y, int x, unsigned char fallback) const;
		[[nodiscard]] array<array<unsigned char, 3>, 2> getChar(int topLeftX, int topLeftY) const;
	
	public:
		SextantDrawing(const vector<vector<unsigned char>>& setDrawing) {
			this->drawing = setDrawing;
		}
		SextantDrawing(const int height, const int width) {
			if (height == 0)
				throw RescaleException("Cannot create a SextantDrawing with height 0");
			this->drawing = vector<vector<unsigned char>>(height, vector<unsigned char>(width, 0));
		}
		[[nodiscard]] int getWidth() const {return this->drawing[0].size();}
		[[nodiscard]] int getHeight() const {return this->drawing.size();}
		void clear();
		void set(int x, int y, unsigned char setTo);
		void trySet(int y, int x, unsigned char setTo);
		void resize(int newX, int newY);
		void insert(int topLeftX, int topLeftY, const SextantDrawing& toCopy, OverrideStyle overrideStyle);
		void render(int topLeftX, int topLeftY) const;
		void debugPrint() const;
};

unsigned char SextantDrawing::getWithFallback(int y, int x, unsigned char fallback) const {
	if (y < 0 || y >= getHeight() || x < 0 || x >= getWidth())
		return fallback;
	else [[likely]]
		return drawing[y][x];
}

void SextantDrawing::clear() {
	for (int y = 0; y < this->getHeight(); y++) {
		for (int x = 0; x < this->getWidth(); x++) {
			this->drawing[y][x] = 0;
		}
	}
}

void SextantDrawing::set(int x, int y, unsigned char setTo) {
	assert((y >= 0 && (uint) y < this->drawing.size()) || !(cerr << "Expected a value in the interval [0, " << this->drawing.size() << "), got " << y << "." << endl));
	assert((x >= 0 && (uint) x < this->drawing[y].size()) || !(cerr << "Expected a value in the interval [0, " << this->drawing[y].size() << "), got " << x << "." << endl));
	this->drawing[y][x] = setTo;
}

void SextantDrawing::trySet(int y, int x, unsigned char setTo) {
	if (y < 0 || y >= getHeight() || x < 0 || x >= getWidth())
		return;
	else [[likely]]
		set(x, y, setTo);
}

void SextantDrawing::resize(int newX, int newY) {
	if (newY == 0)
		throw RescaleException("Cannot resize a SextantDrawing to height 0");
	this->drawing.resize(newY);
	for (int y = 0; y < this->getHeight(); y++) {
		this->drawing[y].resize(newX);
	}
}

array<array<unsigned char, 3>, 2> SextantDrawing::getChar(int topLeftX, int topLeftY) const {
	return {{
		{{
			getWithFallback(topLeftY, topLeftX, 0),
			getWithFallback(topLeftY+1, topLeftX, 0),
			getWithFallback(topLeftY+2, topLeftX, 0)
		}},
		{{
			getWithFallback(topLeftY, topLeftX+1, 0),
			getWithFallback(topLeftY+1, topLeftX+1, 0),
			getWithFallback(topLeftY+2, topLeftX+1, 0)
		}}
	}};
}

// copies toCopy onto this drawing
// topLeft X and Y are in drawing spaces, not characters
void SextantDrawing::insert(int topLeftX, int topLeftY, const SextantDrawing& toCopy,
                            const OverrideStyle overrideStyle = OverrideStyle::Priority) {
	for (int y = 0; y < min(this->getHeight() - topLeftY, toCopy.getHeight()); y++) {
		for (int x = 0; x < min(this->getWidth() - topLeftX, toCopy.getWidth()); x++) {
			//if (this->getWithFallback(y+topLeftY, x+topLeftX, 0) == 0)
			bool doSet;

			switch (overrideStyle) {
				case OverrideStyle::Always:
					doSet = true;
					break;
				case OverrideStyle::Nonzero:
					doSet = this->getWithFallback(y+topLeftY, x+topLeftX, 0) == 0;
					break;
				case OverrideStyle::Priority:
					doSet = this->getWithFallback(y+topLeftY, x+topLeftX, 0) < toCopy.drawing[y][x];
					break;
				case OverrideStyle::Error:
					if (this->getWithFallback(y+topLeftY, x+topLeftX, 0) != 0) {
						throw OverrideException("Override attempted with OverrideStyle::Error set");
					} else {
						doSet = true;
					}
					break;
				default:
					assert(false); // makes GCC shut up
			}

			if (doSet)
				this->set(x+topLeftX, y+topLeftY, toCopy.drawing[y][x]);
		}
	}
}

// top left is specified in characters
void SextantDrawing::render(int topLeftX, int topLeftY) const {
	//print2DVector(this->drawing);
	static wstring_convert<codecvt_utf8<wchar_t>> utf8_conv;
	for (int y = 0; y < this->getHeight(); y += 3) {
		for (int x = 0; x < this->getWidth(); x += 2) {
			auto asArray = getChar(x, y);
			unsigned char maxVal = 0;
			for (auto a: asArray) {
				for (auto b: a) {
					maxVal = max(maxVal, b);
				}
			}
			attrset(COLOR_PAIR(maxVal));

			if (maxVal != 0) {
				mvaddstr(round(y/3) + topLeftY, round(x/2) + topLeftX,
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
