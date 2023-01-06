
#pragma once

#include <map>
#include <main.hpp>

class ConfigFile {
	public:
		// Load success?
		bool valid;
		
		// HTTP server port, 1-65535, default = 8080.
		int httpPort;
		// TODO: Websocket server port, 1-65535, default = 6969.
		
		// No. of FFT bands, 10-200.
		int fftBandCount;
		// No. of FFT samples sent per second, 1-200.
		int fftRate;
		// Timespan of an FFT sample, always >= 1/fftRate.
		float fftTimespan;
		
		// Default constructor: Valid, default values.
		ConfigFile();
		// Load from file.
		ConfigFile(std::string path);
		// Set default values.
		void setDefaults();
		// Save to file.
		void save(std::string path);
};

class Song {
	public:
		// Load success?
		bool valid;
		
		// Song ID.
		uint id;
		// Song queue index.
		uint index;
		// Icon URL.
		std::string iconUrl;
		// Song name.
		std::string name;
		// Download progress as fraction.
		FpType dlProg;
		// Whether the song is being converted.
		bool isConv;
		// Duration in seconds.
		FpType duration;
		// Current playback time in seconds.
		FpType currentTime;
		// Duration string in hh:mm:ss.
		std::string durationStr;
		
		// Default constructor.
		Song();
		// Load song from JSON.
		Song(json &object);
		// Save to song_meta folder.
		void save();
		// Delete from song_meta and song folders.
		void remove();
		// Convert song to JSON.
		json toJson();
		
		// Gets teh STRING DURATION for theee seconds count.
		static std::string stringDuration(FpType seconds);
		// Load from song_meta folder.
		static Song load(uint id);
		// Load all songs from song_meta folder.
		static std::map<uint, Song> loadAll();
};
