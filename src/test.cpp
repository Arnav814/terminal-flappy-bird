#include "types.cpp"

using namespace std;

void func() {
	for (auto it: CoordIterator(SextantCoord(10, 10), SextantCoord(20, 15))) {
		it.print();
	}
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
	func();
}

