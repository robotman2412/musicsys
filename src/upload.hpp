
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <boost/process.hpp>
namespace proc = boost::process;

#include <wsserver.hpp>
#include <data.hpp>

extern int64_t micros();

// A WebSocket based uploading class using ffmpeg for final conversion.
// On error, automatically frees resources used.
class Upload {
	protected:
		// This upload to be ignored if false.
		bool valid;
		
		// Expected incoming size.
		size_t expected;
		// Current incoming size.
		size_t curSize;
		// Temporary path.
		std::string path;
		// Output path.
		std::string outPath;
		// Output file handle.
		std::ofstream fd;
		// Output song ID.
		uint id;
		// Child process used to recode file to mp3.
		proc::child child;
		// Is currently transcoding?
		bool isTranscoding;
		// Has length been checked?
		bool lengthChecked;
		// The song being produced.
		Song song;
		// Mutexxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		std::mutex mtx;
		// Time of last message.
		volatile int64_t msgTime;
		
		// Abort the upload.
		void abort();
		// Decode a bit to determine SONG LENGTH.
		void determineLength();
		
	public:
		// Empty.
		Upload();
		// New upload with given song ID and initial message.
		Upload(uint id, Messager socket, json initialMsg);
		// New upload from local file.
		Upload(uint id, std::string path);
		// Deconstrur.
		~Upload();
		
		// Handles an incoming message.
		// Returns whether this uploads needs to be handled any further.
		bool handleMessage(Messager socket, json data);
		// Whether this uploads needs to be handled any further.
		bool isAlive();
		// Construct and return song meta for this download.
		Song getSong();
};

extern std::vector<std::string> importPaths;

// Start the importer thread that searches the import paths every second or so.
void startImporter();
// Stop the importer thread.
void stopImporter();
