#ifndef BACKGROUNDGEN_CPP
#define BACKGROUNDGEN_CPP

#include <array>
#include <numbers>
#include <random>

#include "moreAssertions.cpp"

namespace BGConstants {
	// number of sines to sum
	const uint nSines = 500;
	array<double, nSines> allCoefs;

	const double spread = 0.1; // lower values produce slower change
	const double smoothness = 0.04; // higher values produce smoother change
	const uint height = 15; // max value
};

void initSines() {
	using namespace BGConstants;

    default_random_engine engine {random_device{}()};
    uniform_real_distribution<double> randomNum(0.0, spread);

	for (uint i = 0; i < nSines; i++) {
		allCoefs[i] = randomNum(engine);
	}
}

uint getBgVal(uint pos) {
	using namespace BGConstants;

	double raw = 0;
	for (double coef: allCoefs) {
		raw += sin(coef * pos) / (nSines * smoothness);
	}

	double normalized = (double) height * (atan(raw) / numbers::pi) + (double) height / 2;

	return round(normalized);
}

#endif /* BACKGROUNDGEN_CPP */
