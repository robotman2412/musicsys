
#include "fft.hpp"

// Default sine provider with increment PI/2.
SineProvider::SineProvider() {
	curCos = 1;
	curSin = 0;
	setIncAngle(M_PI * 0.5);
}

// Update increment as angle.
SineProvider::SineProvider(float angle) {
	curCos = 1;
	curSin = 0;
	setIncAngle(angle);
}

// Update increment as frequency.
SineProvider::SineProvider(float sineHertz, float sampleHertz) {
	setIncFreq(sineHertz, sampleHertz);
}


// Update increment as angle.
void SineProvider::setIncAngle(float angle) {
	incCos = cosf(angle);
	incSin = sinf(angle);
}

// Update increment as frequency.
void SineProvider::setIncFreq(float sineHertz, float sampleHertz) {
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
	float cos0 = curCos, sin0 = curSin;
	
	// Fancy multiplication.
	curCos = incCos * cos0 - incSin * sin0;
	curSin = incSin * cos0 + incCos * sin0;
}



// Rolling sum initialised with default values.
template <typename Num, size_t COUNT>
RollingSum<Num, COUNT>::RollingSum() {
	buffer = new Num[COUNT];
	index  = 0;
	clear();
}

// Rolling sum initialised with specific values.
template <typename Num, size_t COUNT>
RollingSum<Num, COUNT>::RollingSum(const Num &value) {
	buffer = new Num[COUNT];
	index  = 0;
	clear(value);
}

// Copy constructor.
template <typename Num, size_t COUNT>
RollingSum<Num, COUNT>::RollingSum(const RollingSum<Num, COUNT> &other) {
	buffer = new Num[COUNT];
	for (size_t i = 0; i < COUNT; i++) {
		buffer[i] = other.buffer[i];
	}
	sum   = other.sum;
	index = other.index;
}

// Destructor.
template <typename Num, size_t COUNT>
RollingSum<Num, COUNT>::~RollingSum() {
	delete[] buffer;
}


// Get current sum.
template <typename Num, size_t COUNT>
Num RollingSum<Num, COUNT>::get() {
	return sum;
}

// Insert new value into sum.
// Returns new sum.
template <typename Num, size_t COUNT>
Num RollingSum<Num, COUNT>::insert(Num next) {
	sum -= buffer[index];
	buffer[index] = next;
	sum += next;
	index = (index + 1) % COUNT;
	return sum;
}

// Clear to default.
// Returns new sum.
template <typename Num, size_t COUNT>
Num RollingSum<Num, COUNT>::clear(Num value) {
	for (size_t i = 0; i < COUNT; i++) {
		buffer[i] = value;
	}
	sum = value * COUNT;
	return sum;
}



// Rolling average initialised with default values.
template <typename Num, size_t COUNT>
RollingAverage<Num, COUNT>::RollingAverage() {}

// Rolling average initialised with specific values.
template <typename Num, size_t COUNT>
RollingAverage<Num, COUNT>::RollingAverage(const Num &value) {
	sum = RollingSum(value);
}

// Copy constructor.
template <typename Num, size_t COUNT>
RollingAverage<Num, COUNT>::RollingAverage(const RollingAverage<Num, COUNT> &other) {
	sum = other.sum;
}


// Get current average.
template <typename Num, size_t COUNT>
Num RollingAverage<Num, COUNT>::get() {
	return sum.get() / COUNT;
}

// Insert new value into average.
// Returns new average.
template <typename Num, size_t COUNT>
Num RollingAverage<Num, COUNT>::insert(Num next) {
	return sum.insert(next) / COUNT;
}

// Clear to default.
// Returns new average.
template <typename Num, size_t COUNT>
Num RollingAverage<Num, COUNT>::clear(Num value) {
	return sum.clear(value) / COUNT;
}


