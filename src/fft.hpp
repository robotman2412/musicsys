
#pragma once

#include <math.h>
#include <stddef.h>
#include <vector>

// Provides sine/cosine with a known increment each time.
// Default is increment of PI/2.
class SineProvider {
	protected:
		// Current sine/cosine values.
		double curCos, curSin;
		// Increment sine/cosine values.
		double incCos, incSin;
		
	public:
		// Result pair.
		struct Pair {
			double cosine;
			double sine;
		};
		
		// Default sine provider with increment PI/2.
		SineProvider();
		// Update increment as angle.
		SineProvider(double angle);
		// Update increment as frequency.
		SineProvider(double sineHertz, double sampleHertz);
		
		// Update increment as angle.
		void setIncAngle(double angle);
		// Update increment as frequency.
		void setIncFreq(double sineHertz, double sampleHertz);
		
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
		std::vector<Num> buffer;
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
		RollingAverage<Num> average;
		// Desired frequency.
		double freq;
		// Current sample rate.
		double sampleRate;
		// Output rate in analisys outputs per second.
		double outputRate;
		// Time span to analyse in seconds, usually longer than output interval.
		// Enforced to never be shorter than output interval.
		double timeSpan;
		// Amount of samples needed for a new analisys point.
		double missing;
		
	public:
		// Make a new FFT with given sine frequency.
		// Output rate in analisys points per second.
		// Time span for analisys in seconds.
		FFT(double freq=440, double sampleRate=44100, double outputRate=60, double timeSpan=1/45.0);
		
		// Update sample rate.
		void setSampleRate(double newRate);
		// Feed new samples and receive FFT data.
		// It is not gauranteed that this will output more than zero points.
		std::vector<Num> feedSamples(size_t sampleCount, Num *samples);
		// Feed new samples and receive FFT data.
		// It is not gauranteed that this will output more than zero points.
		std::vector<Num> feedSamples(std::vector<Num> samples);
		// Feed new samples and receive FFT data.
		// It is not gauranteed that this will output more than zero points.
		std::vector<Num> feedSamples(double songTime, std::vector<double> &times, size_t sampleCount, Num *samples);
		// Feed new samples and receive FFT data.
		// It is not gauranteed that this will output more than zero points.
		std::vector<Num> feedSamples(double songTime, std::vector<double> &times, std::vector<Num> samples);
};

// A single spectrum analisys output.
template <typename Num>
struct FFTData {
	std::vector<Num> coeff;
	double songTime;
};

// Multiple channel's worth of FFT.
template <typename Num> class FFTSpectrum {
	protected:
		// Internal FFT calculators.
		std::vector<FFT<Num>> channels;
		// Lowest frequency.
		double freqLow;
		// Highest frequency.
		double freqHigh;
		// Amount of channels.
		size_t channelCount;
		// Thread that handles incoming FFT data.
		
	public:
		// Make a new FFT spectrum.
		// Output rate in analisys points per second.
		// Time span for analisys in seconds.
		FFTSpectrum(double freqLow=20, double freqHigh=5000, size_t channelCount=100);
		
		// Update sample rate.
		void setSampleRate(double newRate);
		// Feed new samples and receive FFT data.
		// It is not gauranteed that this will output more than zero points.
		std::vector<FFTData<Num>> feedSamples(double songTime, size_t sampleCount, Num *samples);
		// Feed new samples and receive FFT data.
		// It is not gauranteed that this will output more than zero points.
		std::vector<FFTData<Num>> feedSamples(double songTime, std::vector<Num> samples);
};

#include "fft.impl.hpp"
