
#pragma once

#include <map>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

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
		double dlProg;
		// Whether the song is being converted.
		bool isConv;
		// Duration in seconds.
		double duration;
		// Current playback time in seconds.
		double currentTime;
		// Duration string in hh:mm:ss.
		std::string durationStr;
		
		// Default constructor.
		Song();
		// Load song from JSON.
		Song(json &object);
		// Save to song_meta folder.
		void save();
		// Convert song to JSON.
		json toJson();
		
		// Load from song_meta folder.
		static Song load(uint id);
		// Load all songs from song_meta folder.
		static std::map<uint, Song> loadAll();
};
