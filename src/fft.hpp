
#pragma once

#include <math.h>
#include <stddef.h>

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
template <typename Num, size_t COUNT> class RollingSum {
	protected:
		// Buffer of numeric type for taking rolling sum.
		Num *buffer;
		// Current index in buffer.
		size_t index;
		// Current sum.
		Num sum;
		
	public:
		// Rolling sum initialised with default values.
		RollingSum();
		// Rolling sum initialised with specific values.
		RollingSum(const Num &value);
		// Copy constructor.
		RollingSum(const RollingSum<Num, COUNT> &other);
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
};

// Rolling average of arbitrary number type.
template <typename Num, size_t COUNT> class RollingAverage {
	protected:
		// Internal rolling sum.
		RollingSum<Num, COUNT> sum;
		
	public:
		// Rolling average initialised with default values.
		RollingAverage();
		// Rolling average initialised with specific values.
		RollingAverage(const Num &value);
		// Copy constructor.
		RollingAverage(const RollingAverage<Num, COUNT> &other);
		
		// Get current average.
		Num get();
		// Insert new value into average.
		// Returns new average.
		Num insert(Num next);
		// Clear to default.
		// Returns new average.
		Num clear(Num value = Num());
};

// Single channel's worth of FFT.
template <typename Num, size_t COUNT> class FFT {
	protected:
		
};
