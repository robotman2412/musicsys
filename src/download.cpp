
#include "download.hpp"
#include <boost/process.hpp>
#include <iostream>

namespace proc = boost::process;

// Main function for manager thread.
void Download::managerMain() {
	// Query metadata.
	json obj = queryMetadata();
	if (error) return;
	
	std::cout << "A\n";
	try {
		output.dlProg   = 0;
		output.isConv   = false;
		output.id       = id;
		output.duration = obj["duration"];
		output.iconUrl  = obj["thumbnail"];
		output.name     = obj["fulltitle"];
		output.valid    = true;
		output.durationStr = Song::stringDuration(output.duration);
	} catch(json::type_error err) {
		std::cout << "JSON type error: " << err.what() << std::endl;
		error = true;
		return;
	}
	
	// Determine output path.
	std::string outPath = "data/songs/" + std::to_string(id) + ".%(ext)s";
	
	std::cout << "Starting download of " << url << " to " << outPath << std::endl;
	
	// Call youtube-dl for downloading.
	boost::process::ipstream pipe;
	std::string binary = "/usr/local/bin/yt-dlp";
	// proc::search_path("youtube-dl")
	// proc::child child(binary, "-x", "--audio-format", "mp3", "--newline", "-o", outPath, url);
	proc::child child(binary, "-x", "--audio-format", "mp3", "--newline", "-o", outPath, url, proc::std_out > pipe);
	
	// Initial validity check.
	if (!child.valid()) {
		error = true;
		std::cout << "PROC CREAT EROR." << std::endl;
		return;
	}
	
	// Wait for something to happen.
	std::string expect = "[download]";
	size_t matches = 0;
	int lastProgress = 0;
	runCallbacks();
	while (child.running()) {
		if (cancelled) {
			child.terminate();
			return;
		}
		usleep(10000);
		
		int progress = 0;
		while (1) {
			int c = pipe.get();
			if (c < 0) break;
			// fputc(c, stdout);
			if (matches == expect.length()) {
				// Ignore spaces.
				if (c == ' ') {
					continue;
				} else if (c >= '0' && c <= '9') {
					progress *= 10;
					progress += c - '0';
				} else {
					if (lastProgress != progress) {
						std::cout << "Progress: " << progress << "%" << std::endl;
						output.dlProg = progress / 100.0;
						runCallbacks();
					}
					matches  = 0;
					progress = 0;
				}
				
			} else {
				// Check for C H A R.
				if (c != expect[matches]) {
					matches = 0;
				} else {
					matches ++;
				}
			}
		}
	}
	
	// Await child.
	child.wait();
	
	// Dump it.
	if (obj["fulltitle"].is_string()) {
		std::cout << "Successfully downloaded " << obj["fulltitle"] << std::endl;
	} else {
		std::cout << "Didn't work." << std::endl;
	}
	
	// Check exit status.
	if (child.exit_code() != 0) {
		error = true;
		std::cout << "Nonzero exit code." << std::endl;
		return;
	}
	
	output.dlProg   = 1;
	output.isConv   = false;
	
	completed = true;
}

// Run all callbacks.
void Download::runCallbacks() {
	for (Callback &cb: callbacks) {
		cb(*this);
	}
}

// Blocking metadata query.
json Download::queryMetadata() {
	std::cout << "Querying " << url << std::endl;
	
	// Call youtube-dl for downloading.
	std::string tmpPath = "data/song_meta/" + std::to_string(id) + ".json.tmp";
	
	std::string binary  = "/usr/local/bin/yt-dlp";
	// proc::search_path("youtube-dl")
	proc::child child(binary, "-j", url, proc::std_out > tmpPath);
	
	// Initial validity check.
	if (!child.valid()) {
		error = true;
		std::cout << "PROC CREAT EROR." << std::endl;
		std::remove(tmpPath.c_str());
		return json();
	}
	
	// Time limit on the query.
	for (size_t i = 0; i < 1000; i++) {
		if (cancelled && child.running()) {
			child.terminate();
			error = true;
			std::remove(tmpPath.c_str());
			return json();
		}
		usleep(10000);
	}
	if (child.running()) {
		child.terminate();
	}
	
	// Check exit status.
	child.wait();
	std::ifstream pipe(tmpPath);
	if (child.exit_code() != 0) {
		error = true;
		std::cout << "Nonzero exit code." << std::endl;
		while (!pipe.eof()) {
			std::cout.put(pipe.get());
		}
		std::cout << std::endl;
		std::remove(tmpPath.c_str());
		return json();
	}
	
	std::cout << "Received metadata, decoding." << std::endl;
	json out;
	try {
		pipe >> out;
	} catch(json::parse_error err) {
		error = true;
		out = json();
		std::cout << "Parse error: " << err.what() << std::endl;
	}
	std::remove(tmpPath.c_str());
	return out;
}


// Empty download.
Download::Download() {
	valid = false;
}

// Download from web link, given new song ID.
Download::Download(std::string url, uint song_id) {
	this->url = url;
	this->id  = song_id;
	cancelled = false;
	completed = false;
	error     = false;
	valid     = true;
	running = true;
	manager   = std::thread([this]() {
		managerMain();
		mtx.lock();
		runCallbacks();
		running = false;
		mtx.unlock();
	});
}


// Whether this download was correctly initialised.
bool Download::isValid() {
	return valid;
}

// Whether this download errored.
bool Download::isError() {
	return error;
}

// Whether this download succeeded.
bool Download::isCompleted() {
	return completed;
}

// Whether the downloader thread is currently running.
bool Download::isRunning() {
	return running;
}

// Get a copy of the song meta generated.
Song Download::getSongMeta() {
	return output;
}

// Cancel download.
// This is a blocking operation.
void Download::cancel() {
	cancelled = true;
	error     = true;
	manager.join();
}

// Adds a callback to the list.
// If it is already finished, the callback is run immediately.
void Download::addCallback(Callback cb) {
	mtx.lock();
	if (running) {
		callbacks.push_back(cb);
	} else {
		cb(*this);
	}
	mtx.unlock();
}
