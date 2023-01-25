
#include "data.hpp"
#include "upload.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>



// Set default config values.
void ConfigFile::setDefaults() {
	valid        = true;
	httpPort     = 8080;
	fftBandCount = 20;
	fftRate      = 30;
	fftTimespan  = 1/30.0;
	serverURL    = "";
}

// Default constructor: Valid, default values.
ConfigFile::ConfigFile() {
	setDefaults();
}

// Load from file.
ConfigFile::ConfigFile(std::string path) {
	try {
		// Read JSON file.
		json obj;
		std::ifstream fd(path);
		fd >> obj;
		
		// Read into the VARIABLES.
		httpPort     = obj["http_port"];
		fftBandCount = obj["fft_band_count"];
		fftRate      = obj["fft_rate"];
		fftTimespan  = obj["fft_timespan"];
		if (obj["server_url"].is_string()) serverURL = obj["server_url"];
		
		// Perform bounds checking.
		if (httpPort < 1 || httpPort > 65535) {
			throw std::logic_error("http_port out of range: " + std::to_string(httpPort) + " vs. limit of 1-65535");
		} else if (fftBandCount < 20 || fftBandCount > 200) {
			throw std::logic_error("fft_band_count out of range: " + std::to_string(fftBandCount) + " vs. limit of 20-200");
		} else if (fftRate < 1 || fftRate > 200) {
			throw std::logic_error("fft_rate out of range: " + std::to_string(fftBandCount) + " vs. limit of 1-200");
		} else if (fftTimespan < 1/fftRate) {
			throw std::logic_error("fft_timespan out of range: " + std::to_string(fftBandCount) + " vs. minimum of 1/fft_rate (" + std::to_string(1.0f / fftRate) + ")");
		}
		
	} catch(std::exception &ex) {
		// On error, set defaults.
		std::cout << "config load failed: " << ex.what() << std::endl;
		setDefaults();
		valid = false;
	}
}

// Save to file.
void ConfigFile::save(std::string path) {
	try {
		// Open file.
		std::ofstream fd(path);
		
		// Create JSON object.
		json obj;
		obj["http_port"]      = httpPort;
		obj["fft_band_count"] = fftBandCount;
		obj["fft_rate"]       = fftRate;
		obj["fft_timespan"]   = fftTimespan;
		obj["server_url"]     = serverURL;
		
		// Store to file.
		fd << obj;
		fd.close();
	} catch(std::exception ex) {
		std::cout << "config save failed: " << ex.what() << std::endl;
	}
}



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
	
	if (object["import_path"].is_string()) {
		// Get imported path, and exclude from new attempts.
		importPath = object["import_path"];
		excludeImport(importPath);
		
	} else {
		// If there is none, copy the song retroactively.
		std::string songPath = "data/songs/" + std::to_string(id) + ".mp3";
		importPath = std::filesystem::absolute("data/import/" + name);
		try {
			std::filesystem::copy_file(songPath, importPath);
		} catch (std::filesystem::filesystem_error x) {
			// Ignored
		}
		excludeImport(importPath);
		
		// Update saved metadata.
		save();
	}
}

// Save to song_meta folder.
void Song::save() {
	json out;
	out["id"]                = id;
	out["name"]              = name;
	out["str_duration"]      = durationStr;
	out["import_path"]       = importPath;
	out["icon_url"]          = iconUrl;
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
	if (importPath.length()) {
		includeImport(importPath);
		std::remove(importPath.c_str());
	}
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

// Double-check shared-folder-copy-file existence.
bool Song::checkImportFile() {
	return std::filesystem::is_regular_file(importPath);
}

// Copy song data to the import folder.
void Song::copyToImportFile() {
	std::string songPath = "data/songs/" + std::to_string(id) + ".mp3";
	importPath = std::filesystem::absolute("data/import/" + name);
	excludeImport(importPath);
	try {
		std::filesystem::copy_file(songPath, importPath);
	} catch (std::filesystem::filesystem_error x) {
		// Ignored
	}
}


// Gets teh STRING DURATION for theee seconds count.
std::string Song::stringDuration(FpType seconds0) {
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
			Song &song = map[id];
			std::cout << "Loaded song " << std::to_string(id) << std::endl;
			
			// If it does not exist (but no filesystem error), delete it.
			if (std::filesystem::exists("data/import") && !std::filesystem::is_regular_file(song.importPath)) {
				std::cout << "Deleted \"" << song.name << "\" because it was removed from shared folder." << std::endl;
				song.valid = false;
				song.remove();
			}
		}
	}
	
	return map;
}
