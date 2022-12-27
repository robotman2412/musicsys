
#pragma once

#include <map>
#include <main.hpp>

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
