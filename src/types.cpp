#ifndef TYPES_CPP
#define TYPES_CPP

#include "moreAssertions.cpp"

struct CharCoord {
	int y; int x;

	CharCoord() {}

	CharCoord(const int y, const int x) {
		this->y = y;
		this->x = x;
	}

	CharCoord operator+(const CharCoord& other) const {
		return CharCoord(this->y + other.y, this->x + other.x);
	}

	CharCoord operator-(const CharCoord& other) const {
		return CharCoord(this->y - other.y, this->x - other.x);
	}

	CharCoord operator*(const int other) const {
		return CharCoord(this->y * other, this->x * other);
	}

	CharCoord operator/(const int other) const {
		return CharCoord(this->y / other, this->x / other);
	}

	bool operator==(const CharCoord& other) const {
		return this->y == other.y && this->x == other.x;
	}

	bool operator!=(const CharCoord& other) const {
		return !(*this == other);
	}

	void print() {
		cout << '(' << this->x << ", " << this->y << ')' << endl;
	}
};

struct SextantCoord {
	int y; int x;

	SextantCoord() {}

	SextantCoord(const int y, const int x) {
		this->y = y;
		this->x = x;
	}

	/*SextantCoord(const SextantCoord& coord) {
		this->y = coord.y;
		this->x = coord.x;
	}

	SextantCoord operator=(SextantCoord other) {
		this->y = other.y;
		this->x = other.x;
		return *this;
	}*/

	SextantCoord(const CharCoord& coord) {
		this->y = coord.y * 3;
		this->x = coord.x * 2;
	}

	SextantCoord operator+(const SextantCoord& other) const {
		return SextantCoord(this->y + other.y, this->x + other.x);
	}

	SextantCoord operator-(const SextantCoord& other) const {
		return SextantCoord(this->y - other.y, this->x - other.x);
	}

	SextantCoord operator*(const int other) const {
		return SextantCoord(this->y * other, this->x * other);
	}

	SextantCoord operator/(const int other) const {
		return SextantCoord(this->y / other, this->x / other);
	}

	bool operator==(const SextantCoord& other) const {
		return this->y == other.y && this->x == other.x;
	}

	bool operator!=(const SextantCoord& other) const {
		return !(*this == other);
	}

	void print() {
		cout << '(' << this->x << ", " << this->y << ')' << endl;
	}
};

template <typename coordType> class CoordIterator {
	private:
		coordType start;
		coordType stop;

	public:
		class InternalIterator {
			private:
				coordType i;
				int yMin;
				int yMax;
			
			public:
				InternalIterator(const int yMin, const int yMax, const coordType& start) {
					this->i = start;
					this->yMin = yMin;
					this-> yMax = yMax;
				}

				bool operator==(const InternalIterator& other) {
					return this->i == other.i;
				}

				bool operator!=(const InternalIterator& other) {
					return this->i != other.i;
				}

				InternalIterator operator++() {
					InternalIterator old = *this;
					this->i.y++;
					if (i.y > this->yMax) {
						this->i.y = this->yMin;
						this->i.x++;
					}
					return old;
				}

				coordType operator*() {
					return this->i;
				}
		};

		// both start and end are inclusive
		CoordIterator(const coordType& start, const coordType& stop) {
			assertLtEq(start.x, stop.x, "Start must be less than or equal to stop");
			assertLtEq(start.y, stop.y, "Start must be less than or equal to stop");
			this->start = coordType(start);
			this->stop = coordType(stop);
		}

		InternalIterator begin() const {
			return InternalIterator(this->start.y, this->stop.y, start);
		};

		InternalIterator end() const {
			return InternalIterator(this->start.y, this->stop.y, coordType(this->start.y, this->stop.x+1));
		};
};

#endif /* TYPES_CPP */
