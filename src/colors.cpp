#ifndef COLORS_CPP
#define COLORS_CPP

#include <unordered_map>
#include <utility>
#include <limits>
#include <exception>
#include <ncursesw/curses.h>

#include "moreAssertions.cpp"

using namespace std;

template <> struct std::hash<pair<unsigned char, unsigned char>> {
	size_t operator()(const pair<unsigned char, unsigned char> chPair) const {
		return (size_t) (chPair.first >> 8) | chPair.second;
	}
};

namespace ColorCache {
	short next = 0;
	unordered_map<pair<unsigned char, unsigned char>, short> storedColors;
};

int getColorPair(unsigned char fg, unsigned char bg) {
	auto tryFind = ColorCache::storedColors.find(make_pair(fg, bg));
	if (tryFind == ColorCache::storedColors.end()) {
		if (ColorCache::next == numeric_limits<decltype(ColorCache::next)>::max())
			throw runtime_error("Reached maximium number of color_pairs.");
		ColorCache::next++;
		init_pair(ColorCache::next, fg, bg);
		ColorCache::storedColors.insert(make_pair(make_pair(fg, bg), ColorCache::next));
		return COLOR_PAIR(ColorCache::next);
	} else [[likely]] {
		return COLOR_PAIR(tryFind->second);
	}
}

#endif /* COLORS_CPP */
