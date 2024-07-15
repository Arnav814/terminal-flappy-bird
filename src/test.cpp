#include <iostream>

#include "backgroundGen.cpp"

using namespace std;

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
	initSines();

	for (uint i = 0; i < 1000; i++) {
		cout << to_string(getBgVal(i)) << '\n';
	}
}

