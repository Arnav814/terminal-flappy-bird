#ifndef COLORS_CPP
#define COLORS_CPP

#include <unordered_map>
#include <utility>
#include <ncursesw/curses.h>

#include "moreAssertions.cpp"

using namespace std;

template <> struct std::hash<pair<unsigned char, unsigned char>> {
	size_t operator()(const pair<unsigned char, unsigned char> chPair) const {
		return (size_t) (chPair.first >> 8) | chPair.second;
	}
};

namespace ColorCache {
	size_t next = 0;
	unordered_map<pair<unsigned char, unsigned char>, size_t> storedColors;
};

int getPair(unsigned char fg, unsigned char bg) {
	auto tryFind = ColorCache::storedColors.find(make_pair(fg, bg));
	if (tryFind == ColorCache::storedColors.end()) {
		ColorCache::next++;
		ColorCache::storedColors.insert(make_pair(make_pair(fg, bg), ColorCache::next));
		return COLOR_PAIR(ColorCache::next);
	} else [[likely]] {
		return COLOR_PAIR(tryFind->second);
	}
}

#endif /* COLORS_CPP */
