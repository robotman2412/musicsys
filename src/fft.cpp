
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
template <typename Num>
RollingSum<Num>::RollingSum(size_t length) {
	buffer = new Num[length];
	this->length = length;
	index  = 0;
	clear();
}

// Rolling sum initialised with specific values.
template <typename Num>
RollingSum<Num>::RollingSum(const Num &value, size_t length) {
	buffer = new Num[length];
	this->length = length;
	index  = 0;
	clear(value);
}

// Copy constructor.
template <typename Num>
RollingSum<Num>::RollingSum(const RollingSum<Num> &other) {
	length = other.length;
	buffer = new Num[length];
	for (size_t i = 0; i < length; i++) {
		buffer[i] = other.buffer[i];
	}
	sum   = other.sum;
	index = other.index;
}

// Destructor.
template <typename Num>
RollingSum<Num>::~RollingSum() {
	delete[] buffer;
}


// Get current sum.
template <typename Num>
Num RollingSum<Num>::get() {
	return sum;
}

// Insert new value into sum.
// Returns new sum.
template <typename Num>
Num RollingSum<Num>::insert(Num next) {
	sum -= buffer[index];
	buffer[index] = next;
	sum += next;
	index = (index + 1) % length;
	return sum;
}

// Clear to default.
// Returns new sum.
template <typename Num>
Num RollingSum<Num>::clear(Num value) {
	for (size_t i = 0; i < length; i++) {
		buffer[i] = value;
	}
	sum = value * length;
	return sum;
}

// Get current length.
template <typename Num>
size_t RollingSum<Num>::getLength() {
	return length;
}



// Rolling average initialised with default values.
template <typename Num>
RollingAverage<Num>::RollingAverage(size_t length) {
	sum = RollingSum(length);
}

// Rolling average initialised with specific values.
template <typename Num>
RollingAverage<Num>::RollingAverage(const Num &value, size_t length) {
	sum = RollingSum(value, length);
}

// Copy constructor.
template <typename Num>
RollingAverage<Num>::RollingAverage(const RollingAverage<Num> &other) {
	sum = other.sum;
}


// Get current average.
template <typename Num>
Num RollingAverage<Num>::get() {
	return sum.get() / sum.getLength();
}

// Insert new value into average.
// Returns new average.
template <typename Num>
Num RollingAverage<Num>::insert(Num next) {
	return sum.insert(next) / sum.getLength();
}

// Clear to default.
// Returns new average.
template <typename Num>
Num RollingAverage<Num>::clear(Num value) {
	return sum.clear(value) / sum.getLength();
}

// Get current length.
template <typename Num>
size_t RollingAverage<Num>::getLength() {
	return sum.getLength();
}



// Make a new FFT with given sine frequency.
template <typename Num>
FFT<Num>::FFT(float freq, float sampleRate, float outputRate, float timeSpan) {
	// Copy raw parameters.
	this->freq       = freq;
	this->sampleRate = sampleRate;
	this->outputRate = outputRate;
	this->timeSpan   = timeSpan;
	
	// Feed frequency into sine provider.
	theSine = SineProvider(freq, sampleRate);
	
	// Calculate size of rolling average.
	size_t samples = timeSpan * newRate;
	average = RollingAverage(Num(), samples);
}


// Update sample rate.
template <typename Num>
void FFT<Num>::setSampleRate(float newRate) {
	if (sampleRate != newRate) {
		// Update sine provider with new frequency.
		sampleRate = newRate;
		theSine.setIncFreq(freq, sampleRate);
		
		// Recalculate size of rolling average.
		size_t samples = timeSpan * newRate;
		average = RollingAverage(Num(), samples);
	}
}

// Feed new samples and receive FFT data.
template <typename Num>
std::vector<Num> FFT<Num>::feedSamples(size_t sampleCount, Num *samples) {
	
}

// Feed new samples and receive FFT data.
template <typename Num>
std::vector<Num> FFT<Num>::feedSamples(std::vector<Num> samples) {
	return feedSamples(samples.size(), &samples[0]);
}

