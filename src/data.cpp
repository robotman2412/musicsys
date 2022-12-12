
#include "data.hpp"
#include <fstream>

// Default constructor.
Song::Song() {
	valid = false;
}

// Load song from JSON.
Song::Song(json &object) {
	id          = object["id"];
	index       = 0;
	iconUrl     = object["icon_url"];
	name        = object["name"];
	dlProg      = object["download_progress"];
	isConv      = object["is_converting"];
	durationStr = object["str_duration"];
	valid       = true;
}

// Save to song_meta folder.
void Song::save() {
	json out;
	out["id"]           = id;
	out["name"]         = name;
	out["str_duration"] = durationStr;
	out["icon_url"]     = iconUrl;
	out["is_converting"]     = isConv;
	out["download_progress"] = dlProg;
	std::string path = std::string("data/song_meta/") + std::to_string(id) + ".json";
	std::ofstream output(path);
	output << out;
	output.flush();
	output.close();
}

// Convert song to JSON.
json Song::toJson() {
	json out;
	out["id"]           = id;
	out["index"]        = index;
	out["name"]         = name;
	out["str_duration"] = durationStr;
	out["duration"]     = duration;
	out["current_time"] = currentTime;
	out["icon_url"]     = iconUrl;
	out["is_converting"]     = isConv;
	out["download_progress"] = dlProg;
	return out;
}


// Load from song_meta folder.
Song Song::load(uint id) {
	std::string path = std::string("data/song_meta/") + std::to_string(id) + ".json";
	std::ifstream input(path);
	if (!input.is_open() || input.fail()) {
		return Song();
	} else {
		json obj;
		input >> obj;
		return Song(obj);
	}
}

// Load all songs from song_meta folder.
std::map<uint, Song> Song::loadAll() {
	std::map<uint, Song> map;
	return map;
}
