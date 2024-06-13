#include <array>
#include <bitset>
#include <iostream>

using namespace std;

char packArray(array<array<unsigned char, 3>, 2>& myArray) {
	return (
		((myArray[0][0] != 0 ? 1 : 0) << 0) +
		((myArray[0][1] != 0 ? 1 : 0) << 1) +
		((myArray[0][2] != 0 ? 1 : 0) << 2) +
		((myArray[1][0] != 0 ? 1 : 0) << 3) +
		((myArray[1][1] != 0 ? 1 : 0) << 4) +
		((myArray[1][2] != 0 ? 1 : 0) << 5)
	);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
	
	array<array<unsigned char, 3>, 2> myArray = {{
		{{
			1, 1, 1
		}},
		{{
			0, 0, 0
		}}
	}};

	bitset<6> asBitset(packArray(myArray));
	cout << (asBitset[0] ? '#' : '_');
	cout << (asBitset[1] ? '#' : '_');
	cout << (asBitset[2] ? '#' : '_');
	cout << (asBitset[3] ? '#' : '_');
	cout << (asBitset[4] ? '#' : '_');
	cout << (asBitset[5] ? '#' : '_');
	cout << endl;

	return 0;
}

