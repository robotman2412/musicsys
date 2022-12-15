
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
	return sum.get() / (ssize_t) sum.getLength();
}

// Insert new value into average.
// Returns new average.
template <typename Num>
Num RollingAverage<Num>::insert(Num next) {
	return sum.insert(next) / (ssize_t) sum.getLength();
}

// Clear to default.
// Returns new average.
template <typename Num>
Num RollingAverage<Num>::clear(Num value) {
	return sum.clear(value) / (ssize_t) sum.getLength();
}

// Get current length.
template <typename Num>
size_t RollingAverage<Num>::getLength() {
	return sum.getLength();
}



// Make a new FFT with given sine frequency.
template <typename Num>
FFT<Num>::FFT(double freq, double sampleRate, double outputRate, double timeSpan) {
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
void FFT<Num>::setSampleRate(double newRate) {
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
	double sampleSpan = sampleRate / outputRate;
	
	// Iterate over incoming samples.
	for (size_t i = 0; i < sampleCount; i++) {
		// When fully filled with samples, take the average.
		if (missing <= 0) {
			out.push_back(average.get());
			missing += sampleSpan;
		}
		
		// Generate a new cosine.
		double cosine = theSine.consume().cosine;
		// Multiply it with the sample.
		Num   sample = samples[i] * cosine;
		// Append the multiplied sample to the rolling average.
		average.insert(sample);
		// Decrement the amount of missing things.
		missing -= 1;
	}
	
	return out;
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<Num> FFT<Num>::feedSamples(std::vector<Num> samples) {
	return feedSamples(samples.size(), samples.data());
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<Num> FFT<Num>::feedSamples(double songTime, std::vector<double> &times, size_t sampleCount, Num *samples) {
	std::vector<Num> out;
	
	// Determine samples per output point.
	double sampleSpan = sampleRate / outputRate;
	
	// Determine time delta per sample.
	double deltaTime  = 1.0 / sampleRate;
	
	// Iterate over incoming samples.
	for (size_t i = 0; i < sampleCount; i++) {
		// When fully filled with samples, take the average.
		if (missing <= 0) {
			out.push_back(average.get());
			times.push_back(songTime);
			missing += sampleSpan;
		}
		
		// Generate a new cosine.
		double cosine = theSine.consume().cosine;
		// Multiply it with the sample.
		Num   sample = samples[i] * cosine;
		// Append the multiplied sample to the rolling average.
		average.insert(sample);
		// Decrement the amount of missing things.
		missing -= 1;
		// The time countening.
		songTime += deltaTime;
	}
	
	return out;
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<Num> FFT<Num>::feedSamples(double songTime, std::vector<double> &times, std::vector<Num> samples) {
	return feedSamples(songTime, times, samples.size(), samples.data());
}


// Make a new FFT spectrum.
// Output rate in analisys points per second.
// Time span for analisys in seconds.
template <typename Num>
FFTSpectrum<Num>::FFTSpectrum(double freqLow, double freqHigh, size_t channelCount) {
	// Store raw parameters.
	this->freqLow      = freqLow;
	this->freqHigh     = freqHigh;
	this->channelCount = channelCount;
	
	// Determine frequency gap between channels.
	double freqGap = (freqHigh - freqLow) / (channelCount - 1);
	
	// Make FFTs within the spectrum.
	for (size_t i = 0; i < channelCount; i++) {
		double freq = freqLow + i * freqGap;
		channels.push_back(FFT<Num>(freq));
	}
}


// Update sample rate.
template <typename Num>
void FFTSpectrum<Num>::setSampleRate(double newRate) {
	for (FFT<Num> &fft: channels) {
		fft.setSampleRate(newRate);
	}
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<FFTData<Num>> FFTSpectrum<Num>::feedSamples(double songTime, size_t sampleCount, Num *samples) {
	std::vector<FFTData<Num>> out;
	FFTData<Num> tmp;
	tmp.coeff.resize(channelCount);
	
	// First channel FFT + time.
	std::vector<double> times;
	std::vector<Num> datas = channels[0].feedSamples(songTime, times, sampleCount, samples);
	
	// Make sure there are enough output datas.
	out.reserve(datas.size());
	while (out.size() < datas.size()) {
		out.push_back(tmp);
	}
	
	// Insert data elements.
	for (size_t y = 0; y < datas.size(); y++) {
		out[y].songTime = times[y];
		out[y].coeff[0] = datas[y];
	}
	
	for (size_t x = 1; x < channelCount; x++) {
		FFT<Num> &channel = channels[x];
		datas = channel.feedSamples(sampleCount, samples);
		
		// Insert data elements.
		for (size_t y = 0; y < datas.size(); y++) {
			out[y].coeff[x] = datas[y];
		}
	}
	
	return out;
}

// Feed new samples and receive FFT data.
// It is not gauranteed that this will output more than zero points.
template <typename Num>
std::vector<FFTData<Num>> FFTSpectrum<Num>::feedSamples(double songTime, std::vector<Num> samples) {
	return feedSamples(songTime, samples.size(), samples.data());
}
