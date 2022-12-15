
#include "player.hpp"
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <functional>

#define BUFFER_SAMPLES 1000000
#define VOLUME_MINIMUM 0
#define VOLUME_MAXIMUM 1.5

// (Owner: player thread) Main function for playback thread.
void MpegPlayer::playerMain() {
	uint8_t raw[2048];
	mp3data_struct mp3_type;
	
	while (isAlive) {
		if (isPlaying && isSetup) {
			// Lock before attempting to play.
			playMtx.lock();
			
			double nextTime = currentTime;
			do {
				// Load bytes from input file.
				size_t num = fread(raw, 1, sizeof(raw), currentFd);
				if (!num) {
					// This is clearly the end.
					isPlaying  = false;
					isFinished = true;
					break;
				}
				
				// Try to decode data from the thing.
				sampleCount = hip_decode_headers(decoder, raw, num, leftBuf, rightBuf, &mp3_type);
				if (sampleCount < 0) {
					isPlaying  = false;
					isPlayable = false;
					break;
				} else if (sampleCount > 0) {
					nextTime += sampleCount / (double) mp3_type.samplerate;
				}
			} while (sampleCount == 0 || nextTime < seekTime);
			
			playMtx.unlock();
			
			if (isPlayable && !isFinished) {
				if (sampleRate != mp3_type.samplerate) {
					sampleRate = mp3_type.samplerate;
					fixSampleRate();
				}
				
				// Invoke the sample callback, if any.
				playMtx.lock();
				if (sampleCallback) {
					sampleCallback(currentTime, sampleCount, leftBuf, rightBuf, sampleRate);
				}
				
				// Update current time.
				lastTime     = currentTime;
				sampleTime   = micros();
				currentTime  = nextTime;
				playDuration = mp3_type.nsamp / (double) mp3_type.samplerate;
				playMtx.unlock();
				
				// Send out the samples.
				combineBuffers();
				sendAudio();
			}
		} else {
			usleep(50000);
		}
	}
}

// (Owner: player thread) Merge left and right into combined.
void MpegPlayer::combineBuffers() {
	double vol = volume * volume;
	for (size_t i = 0; i < sampleCount; i++) {
		combinedBuf[i*2+0] = leftBuf [i] * vol;
		combinedBuf[i*2+1] = rightBuf[i] * vol;
	}
}

// (Owner: player thread) Send samples from buffers to PulseAudio.
void MpegPlayer::sendAudio() {
	int error;
	if (pa_simple_write(paCtx, combinedBuf, sampleCount*4, &error) < 0) {
		fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
	}
}

// (Owner: player thread) Fix sample rate by re-opening PulseAudio connection.
void MpegPlayer::fixSampleRate() {
	playMtx.lock();
	
	pa_simple_free(paCtx);
	
	const pa_sample_spec ss = {
		.format   = PA_SAMPLE_S16NE,
		.rate     = sampleRate,
		.channels = 2
	};
	
	/* Create a new playback stream */
	int error;
	paCtx = pa_simple_new(NULL, "musicsys", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error);
	if (!paCtx) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		throw std::bad_alloc();
	}
	playMtx.unlock();
}


// Creates an empty MP3 players.
MpegPlayer::MpegPlayer():
	isSetup  (false),
	isPlaying(false),
	isAlive  (true) {
	isFinished   = false;
	currentTime  = 0;
	lastTime     = 0;
	playDuration = 0;
	currentFd    = NULL;
	leftBuf      = new int16_t[BUFFER_SAMPLES];
	rightBuf     = new int16_t[BUFFER_SAMPLES];
	combinedBuf  = new int16_t[BUFFER_SAMPLES * 2];
	volume       = 1;
	sampleTime   = micros();
	
	sampleRate = 44100;
	const pa_sample_spec ss = {
		.format   = PA_SAMPLE_S16NE,
		.rate     = sampleRate,
		.channels = 2
	};
	
	/* Create a new playback stream */
	int error;
	paCtx = pa_simple_new(NULL, "musicsys", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error);
	if (!paCtx) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		throw std::bad_alloc();
	}
	
	player = new std::thread(&MpegPlayer::playerMain, this);
}

// Stops playback and destroys MP3 player.
MpegPlayer::~MpegPlayer() {
	clear();
	isAlive = false;
	player->join();
	if (currentFd) fclose(currentFd);
	
	pa_simple_free(paCtx);
	delete[] leftBuf;
	delete[] rightBuf;
	delete[] combinedBuf;
}


// Load an MP3 file for playing.
void MpegPlayer::loadFile(std::string path) {
	clear();
	playMtx.lock();
	
	isPlaying = false;
	currentFd = fopen(path.c_str(), "rb");
	if (currentFd) {
		isSetup    = true;
		isPlayable = true;
		decoder    = hip_decode_init();
	} else {
		isSetup    = false;
		isPlayable = false;
	}
	currentTime = 0;
	lastTime    = 0;
	
	playMtx.unlock();
}


// Start playback.
void MpegPlayer::start() {
	playMtx.lock();
	isPlaying = isSetup;
	playMtx.unlock();
}

// Pause playback.
void MpegPlayer::pause() {
	playMtx.lock();
	isPlaying = false;
	playMtx.unlock();
}

// Stop playback and forget current file.
void MpegPlayer::clear() {
	playMtx.lock();
	
	if (isSetup) {
		fclose(currentFd);
		hip_decode_exit(decoder);
	}
	
	isFinished  = false;
	isSetup     = false;
	isPlaying   = false;
	currentFd   = NULL;
	currentTime = 0;
	lastTime    = 0;
	seekTime    = 0;
	
	playMtx.unlock();
}

// Current status of the player.
MpegPlayer::Status MpegPlayer::getStatus() {
	if (!isSetup) return IDLE;
	if (isPlaying) return PLAYING;
	if (isFinished) return FINISHED;
	if (isPlayable) return PAUSED;
	return ERRORED;
}

// Acknowledge finished status and reset to idle status.
void MpegPlayer::acknowledge() {
	if (isFinished) {
		isFinished = false;
		clear();
	}
}


// Determine duration in seconds.
// Returns NaN if unknown.
double MpegPlayer::duration() {
	return playDuration;
}

// Seek to point in seconds.
// Returns point seeked to.
double MpegPlayer::seek(double to) {
	std::lock_guard lock(playMtx);
	if (!isSetup) return 0;
	
	// Clamp range.
	if (to < 0) to = 0;
	if (to > playDuration) to = playDuration;
	
	if (to < currentTime) {
		// Reverse CMD.
		fseek(currentFd, 0, SEEK_SET);
		hip_decode_exit(decoder);
		decoder = hip_decode_init();
		
		isFinished  = false;
		currentTime = 0;
		lastTime    = 0;
	}
	
	// Forward CMD.
	seekTime = to;
	
	return to;
}

// Seek to point in seconds.
// Returns point seeked to.
double MpegPlayer::tell() {
	std::lock_guard lock(playMtx);
	
	if (currentTime < seekTime) {
		return seekTime;
	} else if (isPlaying) {
		return lastTime + (micros() - sampleTime) / 1000000.0;
	} else {
		return currentTime;
	}
}

// Set the new desired volume.
void MpegPlayer::setVolume(double volume) {
	playMtx.lock();
	if (volume > VOLUME_MAXIMUM) volume = VOLUME_MAXIMUM;
	if (volume < VOLUME_MINIMUM) volume = VOLUME_MINIMUM;
	this->volume = volume;
	playMtx.unlock();
}

// Get the current volume.
double MpegPlayer::getVolume() {
	return volume;
}
