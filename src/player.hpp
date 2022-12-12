
#pragma once

#include <stdio.h>

#include <string>
#include <thread>
#include <mutex>
#include <atomic>

#include <lame/lame.h>
#include <pulse/simple.h>
#include <pulse/error.h>

// A C++ wrapper for pulseaudio + LAME based MP3 playback.
// Creates it's own threads for doing so.
class MpegPlayer {
	protected:
		// (Owner: mutex) File descriptor containing current MP3 file, if any.
		FILE        *currentFd;
		// (Owner: mutex) PulseAudio context.
		pa_simple   *paCtx;
		// (Owner: mutex) Decoder context for currently playing file.
		hip_t        decoder;
		// Thread used for playback.
		std::thread *player;
		// Mutex used to synchronise with player thread.
		std::mutex   playMtx;
		
		// Whether the playback thread should stay alive.
		volatile bool  isAlive;
		// Whether the playback is currently set up.
		volatile bool  isSetup;
		// Whether it is playing.
		volatile bool  isPlaying;
		// Whether it is finished playing.
		volatile bool  isFinished;
		// True when there are no errors and it is not the end of the file.
		volatile bool  isPlayable;
		// Duration, or NaN if unknown.
		volatile float playDuration;
		// Current playback time.
		volatile float currentTime;
		// Current playback volume.
		volatile float volume;
		// Current playback sample rate.
		volatile float sampleRate;
		
		// (Owner: player thread) Amount of samples in the buffers.
		ssize_t     sampleCount;
		// (Owner: player thread) Buffer for left channel.
		int16_t    *leftBuf;
		// (Owner: player thread) Buffer for right channel.
		int16_t    *rightBuf;
		// (Owner: player thread) Buffer for combined channel.
		int16_t    *combinedBuf;
		
		// (Owner: player thread) Main function for playback thread.
		void playerMain();
		// (Owner: player thread) Merge left and right into combined.
		void combineBuffers();
		// (Owner: player thread) Send samples from buffers to PulseAudio.
		void sendAudio();
		// (Owner: player thread) Fix sample rate by re-opening PulseAudio connection.
		void fixSampleRate();
		
	public:
		enum Status {
			PLAYING,
			PAUSED,
			FINISHED,
			ERRORED,
			IDLE
		};
		
		// Creates an empty MP3 players.
		MpegPlayer();
		// Stops playback and destroys MP3 player.
		virtual ~MpegPlayer();
		
		// Load an MP3 file for playing.
		void loadFile(std::string path);
		
		// Start playback.
		void start();
		// Pause playback.
		void pause();
		// Stop playback and forget current file.
		void clear();
		// Current status of the player.
		Status getStatus();
		// Acknowledge finished status and reset to idle status.
		void acknowledge();
		
		// Determine duration in seconds.
		// Returns NaN if unknown.
		float duration();
		// Seek to point in seconds.
		// Returns point seeked to.
		float seek(float to);
		// Tells current time.
		float tell();
		
		// Set the new desired volume.
		void setVolume(float volume);
		// Get the current volume.
		float getVolume();
};
