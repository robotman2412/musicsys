
#pragma once

#include <math.h>
#include <stddef.h>
#include <vector>

// Provides sine/cosine with a known increment each time.
// Default is increment of PI/2.
class SineProvider {
	protected:
		// Current sine/cosine values.
		float curCos, curSin;
		// Increment sine/cosine values.
		float incCos, incSin;
		
	public:
		// Result pair.
		struct Pair {
			float cosine;
			float sine;
		};
		
		// Default sine provider with increment PI/2.
		SineProvider();
		// Update increment as angle.
		SineProvider(float angle);
		// Update increment as frequency.
		SineProvider(float sineHertz, float sampleHertz);
		
		// Update increment as angle.
		void setIncAngle(float angle);
		// Update increment as frequency.
		void setIncFreq(float sineHertz, float sampleHertz);
		
		// Peek sine/cosine pair.
		Pair peek();
		// Consume current pair; peek() followed by increment().
		Pair consume();
		// Increment to next state.
		void increment();
};

// Rolling sum of arbitrary number type.
template <typename Num> class RollingSum {
	protected:
		// Buffer of numeric type for taking rolling sum.
		Num *buffer;
		// Current index in buffer.
		size_t index;
		// Current length.
		size_t length;
		// Current sum.
		Num sum;
		
	public:
		// Rolling sum initialised with default values.
		RollingSum(size_t length=10);
		// Rolling sum initialised with specific values.
		RollingSum(const Num &value, size_t length=10);
		// Copy constructor.
		RollingSum(const RollingSum<Num> &other);
		// Destructor.
		~RollingSum();
		
		// Get current sum.
		Num get();
		// Insert new value into sum.
		// Returns new sum.
		Num insert(Num next);
		// Clear to default.
		// Returns new sum.
		Num clear(Num value = Num());
		// Get current length.
		size_t getLength();
};

// Rolling average of arbitrary number type.
template <typename Num> class RollingAverage {
	protected:
		// Internal rolling sum.
		RollingSum<Num> sum;
		
	public:
		// Rolling average initialised with default values.
		RollingAverage(size_t length=10);
		// Rolling average initialised with specific values.
		RollingAverage(const Num &value, size_t length=10);
		// Copy constructor.
		RollingAverage(const RollingAverage<Num> &other);
		
		// Get current average.
		Num get();
		// Insert new value into average.
		// Returns new average.
		Num insert(Num next);
		// Clear to default.
		// Returns new average.
		Num clear(Num value = Num());
		// Get current length.
		size_t getLength();
};

// Single channel's worth of FFT.
template <typename Num> class FFT {
	protected:
		// Cosine calculator.
		SineProvider theSine;
		// Rolling average used to calculate FFT.
		RollingAverage average;
		// Desired frequency.
		float freq;
		// Current sample rate.
		float sampleRate;
		// Output rate in analisys outputs per second.
		float outputRate;
		// Time span to analyse in seconds, usually longer than output interval.
		float timeSpan;
		
	public:
		// Make a new FFT with given sine frequency.
		// Output rate in analisys points per second.
		// Time span for analisys in seconds.
		FFT(float freq=440, float sampleRate=44100, float outputRate=60, float timeSpan=1/30.0);
		
		// Update sample rate.
		void setSampleRate(float newRate);
		// Feed new samples and receive FFT data.
		std::vector<Num> feedSamples(size_t sampleCount, Num *samples);
		// Feed new samples and receive FFT data.
		std::vector<Num> feedSamples(std::vector<Num> samples);
};

// Multiple channel's worth of FFT.
template <typename Num> class FFTSpectrum {
	
};
