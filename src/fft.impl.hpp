
#include "fft.hpp"
#include <iostream>
// Rolling sum initialised with default values.
template <typename Num>
RollingSum<Num>::RollingSum(size_t length) {
	buffer.resize(length);
	this->length = length;
	index  = 0;
	clear();
}

// Rolling sum initialised with specific values.
template <typename Num>
RollingSum<Num>::RollingSum(const Num &value, size_t length) {
	buffer.resize(length);
	this->length = length;
	index  = 0;
	clear(value);
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
	sum = RollingSum<Num>(length);
}

// Rolling average initialised with specific values.
template <typename Num>
RollingAverage<Num>::RollingAverage(const Num &value, size_t length) {
	sum = RollingSum<Num>(value, length);
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
	// Check parameters.
	if (timeSpan < 1/outputRate) {
		timeSpan = 1/outputRate;
	}
	
	// Copy raw parameters.
	this->freq       = freq;
	this->sampleRate = sampleRate;
	this->outputRate = outputRate;
	this->timeSpan   = timeSpan;
	missing = 0;
	
	// Feed frequency into sine provider.
	theSine = SineProvider(freq, sampleRate);
	
	// Calculate size of rolling average.
	size_t samples = timeSpan * sampleRate;
	average = RollingAverage<Num>(Num(), samples);
}


// Update sample rate.
template <typename Num>
void FFT<Num>::setSampleRate(float newRate) {
	if (sampleRate != newRate) {
		// Update sine provider with new frequency.
		sampleRate = newRate;
		theSine.setIncFreq(freq, sampleRate);
		
		// Recalculate size of rolling average.
		size_t samples = timeSpan * sampleRate;
		average = RollingAverage<Num>(Num(), samples);
	}
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<Num> FFT<Num>::feedSamples(size_t sampleCount, Num *samples) {
	std::vector<Num> out;
	
	// Determine samples per output point.
	size_t sampleSpan = sampleRate / outputRate;
	
	// Iterate over incoming samples.
	for (size_t i = 0; i < sampleCount; i++) {
		// When fully filled with samples, take the average.
		if (missing == 0) {
			out.push_back(average.get());
			missing = sampleSpan;
		}
		
		// Generate a new cosine.
		float cosine = theSine.consume().cosine;
		// Multiply it with the sample.
		Num   sample = samples[i] * cosine;
		// Append the multiplied sample to the rolling average.
		average.insert(sample);
		// Decrement the amount of missing things.
		missing --;
	}
	
	return out;
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<Num> FFT<Num>::feedSamples(std::vector<Num> samples) {
	return feedSamples(samples.size(), samples.data());
}



// Make a new FFT spectrum.
// Output rate in analisys points per second.
// Time span for analisys in seconds.
template <typename Num>
FFTSpectrum<Num>::FFTSpectrum(float freqLow, float freqHigh, size_t channelCount) {
	// Store raw parameters.
	this->freqLow      = freqLow;
	this->freqHigh     = freqHigh;
	this->channelCount = channelCount;
	
	// Determine frequency gap between channels.
	float freqGap = (freqHigh - freqLow) / (channelCount - 1);
	
	// Make FFTs within the spectrum.
	for (size_t i = 0; i < channelCount; i++) {
		float freq = freqLow + i * freqGap;
		channels.push_back(FFT<Num>(freq));
	}
}


// Update sample rate.
template <typename Num>
void FFTSpectrum<Num>::setSampleRate(float newRate) {
	for (FFT<Num> &fft: channels) {
		fft.setSampleRate(newRate);
	}
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<std::vector<Num>> FFTSpectrum<Num>::feedSamples(size_t sampleCount, Num *samples) {
	std::vector<std::vector<Num>> out;
	std::vector<Num> tmp;
	tmp.resize(channelCount);
	
	for (size_t x = 0; x < channelCount; x++) {
		FFT<Num> &channel = channels[x];
		std::vector<Num> datas = channel.feedSamples(sampleCount, samples);
		
		// Make sure there are enough output datas.
		while (out.size() < datas.size()) {
			out.push_back(tmp);
		}
		
		// Insert data elements.
		for (size_t y = 0; y < datas.size(); y++) {
			out[y][x] = datas[y];
		}
	}
	
	return out;
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<std::vector<Num>> FFTSpectrum<Num>::feedSamples(std::vector<Num> samples) {
	return feedSamples(samples.size(), samples.data());
}
