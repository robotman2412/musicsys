
#include "data.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

// Default constructor.
Song::Song() {
	valid = false;
}

// Load song from JSON.
Song::Song(json &object) {
	valid = true;
	if (object["id"].is_number()) id = object["id"];
	else valid = false;
	index = 0;
	if (object["icon_url"].is_string()) iconUrl = object["icon_url"];
	if (object["name"].is_string()) name = object["name"];
	else valid = false;
	dlProg = 1;
	isConv = false;
	if (object["str_duration"].is_string()) durationStr = object["str_duration"];
	else valid = false;
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

// Delete from song_meta and song folders.
void Song::remove() {
	std::string path = std::string("data/song_meta/") + std::to_string(id) + ".json";
	std::remove(path.c_str());
	path = std::string("data/songs/") + std::to_string(id) + ".mp3";
	std::remove(path.c_str());
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


// Gets teh STRING DURATION for theee seconds count.
std::string Song::stringDuration(float seconds0) {
	int sec = seconds0+0.5;
	int min = sec / 60;
	sec %= 60;
	int hour = min / 60;
	min %= 60;
	
	char tmp[32];
	if (hour) {
		snprintf(tmp, sizeof(tmp), "%02d:%02d:%02d", hour, min, sec);
	} else {
		snprintf(tmp, sizeof(tmp), "%02d:%02d", min, sec);
	}
	
	return std::string(tmp);
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

static bool isValidMetaPath(std::filesystem::directory_entry entry, uint &id) {
	// Extract filename from path.
	std::string path  = entry.path();
	size_t      index = path.find_last_of('/', path.size()-1);
	if (index >= path.length()) index = 0;
	std::string name  = path.substr(index+1);
	
	// Enforce filename ends with '.json'.
	if (name.length() < 6) return false;
	if (name.substr(name.length()-5, 5) != ".json") return false;
	
	// Enforce filename starts with number.
	std::string raw_num = name.substr(0, name.length()-5);
	if (raw_num.find_first_not_of("0123456789", 0) < raw_num.length()) return false;
	
	// Parse number.
	id = std::stoi(raw_num);
	return true;
}

// Load all songs from song_meta folder.
std::map<uint, Song> Song::loadAll() {
	std::map<uint, Song> map;
	
	for (const auto &entry: std::filesystem::directory_iterator("data/song_meta")) {
		uint id = 0;
		if (entry.is_regular_file() && isValidMetaPath(entry, id)) {
			map[id] = Song::load(id);
			std::cout << "Loaded song " << std::to_string(id) << std::endl;
		}
	}
	
	return map;
}
