
#pragma once

#include <string>
#include <thread>
#include <functional>
#include <mutex>

#include "data.hpp"

// An asynchronous downloader using yt-dlp.
class Download {
	public:
		// Callback for when finish, whether error or success.
		typedef std::function<void(Download&)> Callback;
		
	protected:
		// To be ignored if false.
		volatile bool valid;
		// Error occurred.
		volatile bool error;
		// Completed successfully.
		volatile bool completed;
		// It is CANCELLED!!!!
		volatile bool cancelled;
		// Whether the manager is currently running.
		volatile bool running;
		
		// URL to download.
		std::string url;
		// Song ID to produce.
		uint id;
		// Song produced.
		Song output;
		
		// Keeps track of yt-dl.
		std::thread manager;
		// Main function for manager thread.
		void managerMain();
		// Run all callbacks.
		void runCallbacks();
		// List of callbacks that will be triggered when the download finishes.
		std::vector<Callback> callbacks;
		// Generic mutex for concurrency.
		std::mutex mtx;
		
		// Blocking metadata query.
		json queryMetadata();
		
	public:
		// Empty download.
		Download();
		// Copy constructor is dangerous.
		Download(Download &) = delete;
		// Download from web link, given new song ID.
		Download(std::string url, uint song_id);
		
		// Whether this download was correctly initialised.
		bool isValid();
		// Whether this download errored.
		bool isError();
		// Whether this download succeeded.
		bool isCompleted();
		// Whether the downloader thread is currently running.
		bool isRunning();
		// Get a copy of the song meta generated.
		Song getSongMeta();
		// Cancel download.
		// This is a blocking operation.
		void cancel();
		// Adds a callback to the list.
		// If it is already finished, the callback is run immediately.
		void addCallback(Callback cb);
};
