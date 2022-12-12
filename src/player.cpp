
#include "player.hpp"
#include <unistd.h>
#include <stdexcept>
#include <iostream>
#include <functional>

#define BUFFER_SAMPLES 44100
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
			
			do {
				// Load bytes from input file.
				size_t num = fread(raw, 1, sizeof(raw), currentFd);
				if (!num) {
					isPlaying  = false;
					isPlayable = false;
					break;
				}
				
				// Try to decode data from the thing.
				sampleCount = hip_decode_headers(decoder, raw, num, leftBuf, rightBuf, &mp3_type);
				if (sampleCount < 0) {
					isPlaying  = false;
					isPlayable = false;
					break;
				}
			} while (sampleCount == 0);
			
			playMtx.unlock();
			
			if (isPlayable) {
				// Send out the samples.
				combineBuffers();
				sendAudio();
				
				// Update current time.
				currentTime += sampleCount / (float) mp3_type.samplerate;
				playDuration = mp3_type.nsamp / (float) mp3_type.samplerate;
			}
		} else {
			usleep(50000);
		}
	}
}

// (Owner: player thread) Merge left and right into combined.
void MpegPlayer::combineBuffers() {
	float vol = volume * volume;
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


// Creates an empty MP3 players.
MpegPlayer::MpegPlayer():
	isSetup  (false),
	isPlaying(false),
	isAlive  (true) {
	player = new std::thread(&MpegPlayer::playerMain, this);
	currentTime  = 0;
	playDuration = 0;
	currentFd    = NULL;
	leftBuf      = new int16_t[BUFFER_SAMPLES];
	rightBuf     = new int16_t[BUFFER_SAMPLES];
	combinedBuf  = new int16_t[BUFFER_SAMPLES * 2];
	volume       = 1;
	
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16NE,
		.rate = 44100,
		.channels = 2
	};
	
	/* Create a new playback stream */
	int error;
	paCtx = pa_simple_new(NULL, "musicsys", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error);
	if (!paCtx) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		throw std::bad_alloc();
	}
}

// Stops playback and destroys MP3 player.
MpegPlayer::~MpegPlayer() {
	clear();
	isAlive = false;
	player->join();
	
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
	
	isSetup   = false;
	isPlaying = false;
	currentFd = NULL;
	
	playMtx.unlock();
}

// Current status of the player.
MpegPlayer::Status MpegPlayer::getStatus() {
	if (!isSetup) return IDLE;
	if (isPlaying) return PLAYING;
	if (isPlayable) return PAUSED;
	return ERRORED;
}


// Determine duration in seconds.
// Returns NaN if unknown.
float MpegPlayer::duration() {
	return playDuration;
}

// Seek to point in seconds.
// Returns point seeked to.
float MpegPlayer::seek(float to) {
	return currentTime;
}

// Seek to point in seconds.
// Returns point seeked to.
float MpegPlayer::tell() {
	return currentTime;
}

// Set the new desired volume.
void MpegPlayer::setVolume(float volume) {
	playMtx.lock();
	if (volume > VOLUME_MAXIMUM) volume = VOLUME_MAXIMUM;
	if (volume < VOLUME_MINIMUM) volume = VOLUME_MINIMUM;
	this->volume = volume;
	playMtx.unlock();
}

// Get the current volume.
float MpegPlayer::getVolume() {
	return volume;
}
