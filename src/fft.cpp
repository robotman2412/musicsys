
#include "fft.hpp"

// Default sine provider with increment PI/2.
SineProvider::SineProvider() {
	curCos = 1;
	curSin = 0;
	setIncAngle(M_PI * 0.5);
}

// Update increment as angle.
SineProvider::SineProvider(double angle) {
	curCos = 1;
	curSin = 0;
	setIncAngle(angle);
}

// Update increment as frequency.
SineProvider::SineProvider(double sineHertz, double sampleHertz) {
	curCos = 1;
	curSin = 0;
	setIncFreq(sineHertz, sampleHertz);
}


// Update increment as angle.
void SineProvider::setIncAngle(double angle) {
	incCos = cosf(angle);
	incSin = sinf(angle);
}

// Update increment as frequency.
void SineProvider::setIncFreq(double sineHertz, double sampleHertz) {
	setIncAngle(M_PI * 2 * sineHertz / sampleHertz);
}


// Peek sine/cosine pair.
SineProvider::Pair SineProvider::peek() {
	return Pair{ .cosine = curCos, .sine = curSin };
}

// Consume current pair; peek() followed by increment().
SineProvider::Pair SineProvider::consume() {
	Pair cur = peek();
	increment();
	return cur;
}

// Increment to next state.
void SineProvider::increment() {
	double cos0 = curCos, sin0 = curSin;
	
	// Fancy multiplication.
	curCos = incCos * cos0 - incSin * sin0;
	curSin = incSin * cos0 + incCos * sin0;
}
