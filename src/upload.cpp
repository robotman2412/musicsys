
#include "upload.hpp"
#include <lame/lame.h>
#include <main.hpp>

static short the_big_dummy_array[100000];
#define RECV_TIMEOUT 5000000

// Abort the upload.
void Upload::abort() {
	std::cout << "Upload " << id << " aborted" << std::endl;
	
	valid = false;
	
	// Delete files.
	std::remove(path.c_str());
	std::remove(outPath.c_str());
}

// Decode a bit to determine SONG LENGTH.
void Upload::determineLength() {
	hip_t decoder = hip_decode_init();
	std::ifstream in(outPath, std::ifstream::binary);
	mp3data_struct mp3;
	int count = 0;
	do {
		char tmp[1024];
		ssize_t read = in.readsome(tmp, sizeof(tmp));
		count = hip_decode1_headers(decoder, (uint8_t*) tmp, read, the_big_dummy_array, the_big_dummy_array, &mp3);
	} while (count == 0);
	hip_decode_exit(decoder);
	song.duration = (FpType) mp3.nsamp / (FpType) mp3.samplerate;
	song.durationStr = Song::stringDuration(song.duration);
}


// Empty.
Upload::Upload() {
	valid = false;
}

// New upload with given song ID.
Upload::Upload(uint id, Messager socket, json msg) {
	valid     = true;
	this->id  = id;
	song.id   = id;
	isTranscoding = false;
	lengthChecked = false;
	
	// Check message type.
	if (!msg["ref"].is_number() || !msg["size"].is_number() || !msg["name"].is_string()) {
		// Invalidate upload.
		std::cout << "Invalid upload request: " << msg << std::endl;
		valid = false;
		return;
	}
	
	// Read message data.
	song.name    = escapeHTML(msg["name"]);
	song.iconUrl = "default_icon.jpg";
	curSize      = 0;
	expected     = msg["size"];
	msgTime      = micros();
	
	// Open file.
	path = "./data/songs/" + std::to_string(id) + ".tmp";
	outPath = "./data/songs/" + std::to_string(id) + ".mp3";
	fd.open(path, std::ofstream::binary);
	
	// Acknowledge upload.
	json out;
	out["upload_prog"]           = json::object();
	out["upload_prog"]["id"]     = id;
	out["upload_prog"]["ref"]    = msg["ref"];
	out["upload_prog"]["status"] = "start";
	socket(out.dump());
}

// New upload from local file.
Upload::Upload(uint id, std::string inPath) {
	valid        = true;
	this->id     = id;
	song.id      = id;
	song.name    = escapeHTML(filename(inPath, false));
	song.iconUrl = "default_icon.jpg";
	song.dlProg  = 1;
	
	// Move it to a temp file.
	std::string tmpPath = "data/songs/" + std::to_string(id) + ".tmp";
	if (!std::filesystem::copy_file(inPath, tmpPath)) {
		abort();
		std::remove(inPath.c_str());
		return;
	}
	std::remove(inPath.c_str());
	
	// Set paths.
	path = tmpPath;
	outPath = "data/songs/" + std::to_string(id) + ".mp3";
	
	// Start transcoding.
	isTranscoding = true;
	song.isConv   = true;
	lengthChecked = false;
	
	// Start child process.
	auto binary = boost::process::search_path("ffmpeg");
	child = boost::process::child(binary, "-i", path, "-movflags", "use_metadata_tags", outPath);
	
	if (!child.valid()) { abort(); return; }
}

// Deconstrur.
Upload::~Upload() {
	std::remove(path.c_str());
	if (!valid) {
		// Not valid song? Delete it!
		json obj;
		obj["delete_song"] = id;
		broadcast(obj.dump());
		return;
	}
	
	std::cout << "Upload " << id << " completed" << std::endl;
}


// Handles an incoming message.
// Returns whether this uploads needs to be handled any further.
bool Upload::handleMessage(Messager socket, json data) {
	std::lock_guard lock(mtx);
	
	if (!valid) return false;
	if (isTranscoding) { abort(); return false; }
	if (micros() > msgTime + RECV_TIMEOUT) { abort(); return false; }
	
	// Check message type.
	if (!data["id"].is_number()) { abort(); return false; }
	if (!data["data"].is_array()) { abort(); return false; }
	
	// Check some BYTES.
	json &arr = data["data"];
	for (size_t i = 0; i < arr.size(); i++) {
		// Check and write each byte.
		if (!arr[i].is_number()) { abort(); return false; }
		fd.put((int) arr[i]);
		
		// Increment received size.
		curSize ++;
	}
	song.dlProg = (FpType) curSize / (FpType) expected;
	
	// Prepare response.
	json out;
	out["upload_prog"] = json::object();
	if (curSize < expected) {
		// Ask for more data.
		out["upload_prog"]["id"]     = id;
		out["upload_prog"]["status"] = "continue";
		
	} else {
		// Formally finish download.
		out["upload_prog"]["id"]     = id;
		out["upload_prog"]["status"] = "done";
	}
	
	if (curSize >= expected) {
		isTranscoding = true;
		song.isConv = true;
		
		// Start child process.
		auto binary = boost::process::search_path("ffmpeg");
		child = boost::process::child(binary, "-i", path, outPath);
		
		if (!child.valid()) { abort(); return false; }
	}
	
	// Broadcast the WIP song.
	out["song_meta"] = song.toJson();
	socket(out.dump());
	msgTime = micros();
	
	return true;
}

// Whether this uploads needs to be handled any further.
bool Upload::isAlive() {
	std::lock_guard lock(mtx);
	if (isTranscoding && !child.running()) {
		if (child.exit_code()) {
			abort();
			return false;
		}
		
		if (!lengthChecked) {
			determineLength();
		}
		
		song.isConv = false;
		song.valid  = valid;
		return false;
	}
	if (!isTranscoding && micros() > msgTime + RECV_TIMEOUT) { abort(); return false; }
	
	return valid;
}

// Construct and return song meta for this download.
Song Upload::getSong() {
	return song;
}



std::vector<std::string> importPaths;

static std::thread importer;
static volatile bool importerAlive;

extern std::map<uint, Upload *> uploads;

// Main method for importer thread.
static void importerMain() {
	while (importerAlive) {
		for (std::string path: importPaths) {
			try {
				for (auto entry: std::filesystem::directory_iterator(path)) {
					if (entry.is_regular_file()) {
						uint id = getNewId();
						uploads[id] = new Upload(id, entry.path());
					}
				}
			} catch (std::filesystem::filesystem_error x) {
				// Ignored
			}
		}
		usleep(1000000);
	}
}

// Start the importer thread that searches the import paths every second or so.
void startImporter() {
	importerAlive = true;
	importer = std::thread(importerMain);
}

// Stop the importer thread.
void stopImporter() {
	importerAlive = false;
	importer.join();
}

