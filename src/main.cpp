
#include "main.hpp"
#include "httpserver.hpp"
#include "wsserver.hpp"
#include "player.hpp"
#include "data.hpp"
#include "download.hpp"
#include "fft.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFSIZE 44100

#include <lame/lame.h>

MpegPlayer player;
Song       nowPlaying;
uint new_id_index = 0;
std::vector<Song> queue;
uint64_t queue_index = 0;
std::map<uint, Song> songs;
std::vector<Download *> downloads;

int main(int argc, char **argv) {
	// Start servers.
	startHttpServer(8080, "./web/", 1);
	startWebsocketServer(6969, 1);
	
	// Load data.
	songs = Song::loadAll();
	
	// Determine next ID to to-be-uploaded songs.
	for (auto &pair: songs) {
		if (pair.first >= new_id_index) {
			new_id_index = pair.first + 1;
		}
	}
	std::cout << "Next song id: " << new_id_index << std::endl;
	
	// Default volume.
	player.setVolume(0.4);
	
	// Send metadata from time to time.
	while (1) {
		sendSongStatus();
		if (player.getStatus() == MpegPlayer::FINISHED) {
			if (queue.size()) {
				// Load more from queue
				Song next = queue[0];
				removeFromQueue(next.index);
				playNow(next.id);
			} else {
				player.acknowledge();
				broadcast("{\"now_playing_nothing\":true}");
				nowPlaying = Song();
			}
		}
		usleep(100000);
	}
	
	// Close servers.
	stopHttpServer();
	stopWebsocketServer();
	return 0;
}

// Handle a new connecting socket.
void handleAccepted(Messager socket) {
	std::cout << "Accepted" << std::endl;
	
	// Send SONG META to new socket.
	for (auto &pair: songs) {
		Song &song = pair.second;
		json obj;
		obj["song_meta"] = song.toJson();
		if (song.valid) socket(obj.dump());
	}
	
	// Send QUEUE META to new socket.
	for (auto &song: queue) {
		json obj;
		obj["song_in_queue_meta"] = song.toJson();
		if (song.valid) socket(obj.dump());
	}
}

// Handle a new incoming message.
void handleMessage(Messager socket, std::string in) {
	std::cout << in << std::endl;
	
	json data = json::parse(in);
	if (data["set_playing"].is_boolean()) {
		// Play/pause command.
		if (data["set_playing"]) player.start();
		else player.pause();
		
		// Broadcast playing status.
		if (player.getStatus() == MpegPlayer::PLAYING) {
			broadcast("{\"playing\":true}");
		} else {
			broadcast("{\"playing\":false}");
		}
		
	} else if (data["play_song_now"].is_number()) {
		// Play song command.
		playNow(data["play_song_now"]);
		
	} else if (data["set_volume"].is_number()) {
		// Set volume command.
		player.setVolume(data["set_volume"]);
		
	} else if (data["add_to_queue"].is_number()) {
		// Add to queue command.
		uint id = data["add_to_queue"];
		if (!songs[id].valid) return;
		
		if (!nowPlaying.valid) {
			// Play now instead.
			playNow(id);
		} else {
			// Add to queue normally.
			addToQueue(id);
		}
	} else if (data["remove_from_queue"].is_number()) {
		// Remove from queue command.
		removeFromQueue(data["remove_from_queue"]);
		
	} else if (data.contains("skip")) {
		// Skip song command.
		skipSong();
		
	} else if (data["submit_song"].is_object()) {
		// Submit song command.
		if (!data["submit_song"]["url"].is_string() || !data["submit_song"]["name"].is_string()) return;
		
		// Create downloader.
		uint id = new_id_index++;
		Download *dl = new Download(data["submit_song"]["url"], id);
		downloads.push_back(dl);
		
		// Make a callback.
		std::string name = data["submit_song"]["name"];
		dl->addCallback([name, id](Download &dl) {
			// On error, delete song.
			if (dl.isError()) {
				deleteSong(id);
				return;
			}
			
			// Get generated song.
			Song thing = dl.getSongMeta();
			// If there is a name specified, use it.
			if (name.length() != 0) thing.name = name;
			
			// Pack into JSON
			json obj;
			obj["song_meta"] = thing.toJson();
			songs[thing.id] = thing;
			broadcast(obj.dump());
			
			// If finished, save to disk.
			if (thing.dlProg >= 0.999) thing.save();
		});
		
	} else if (data["delete_song"].is_number()) {
		// Delete song command.
		uint id = data["delete_song"];
		deleteSong(id);
		
	}
}

// Broadcast song status.
void sendSongStatus() {
	nowPlaying.currentTime = player.tell();
	nowPlaying.duration    = player.duration();
	json msg;
	bool playing = player.getStatus() == MpegPlayer::PLAYING;
	if (playing) {
		msg["song_now_playing"] = nowPlaying.toJson();
	}
	msg["playing"] = playing;
	msg["set_volume"] = player.getVolume();
	broadcast(msg.dump());
}

// Load and play song by ID.
void playNow(uint id) {
	if (!songs[id].valid) return;
	
	std::string path = "data/songs/" + std::to_string(id) + ".mp3";
	player.loadFile(path);
	player.start();
	if (player.getStatus() == MpegPlayer::PLAYING) {
		// This indicates successful loading.
		nowPlaying = songs[id];
		nowPlaying.currentTime = 0;
		json msg;
		msg["song_now_playing"] = nowPlaying.toJson();
		msg["playing"] = true;
		broadcast(msg.dump());
	}
}

// Skips the current song.
void skipSong() {
	if (queue.size()) {
		// Load more from queue
		Song next = queue[0];
		removeFromQueue(next.index);
		playNow(next.id);
	} else {
		// Stop playing.
		player.clear();
		broadcast("{\"now_playing_nothing\":true}");
	}
}

// Delete song by ID.
void deleteSong(uint id) {
	// Find song in list.
	auto iter = songs.find(id);
	if (iter == songs.end()) return;
	
	// Remove from queue.
	for (ssize_t i = queue.size()-1; i >= 0; i--) {
		if (queue[i].id == id) {
			json obj;
			obj["remove_from_queue"] = queue[i].index;
			broadcast(obj.dump());
			queue.erase(queue.begin() + i, queue.end());
		}
	}
	
	// If playing, skip.
	skipSong();
	
	// Broadcast the deletion.
	json obj;
	obj["delete_song"] = id;
	broadcast(obj.dump());
	
	// If found, delete.
	iter->second.remove();
	songs.erase(iter);
}

// Add song to queue by ID.
void addToQueue(uint id) {
	// Add to queue.
	Song entry  = songs[id];
	entry.index = queue_index++;
	queue.push_back(entry);
	
	// Broadcast this fact.
	json obj;
	obj["song_in_queue_meta"] = entry.toJson();
	broadcast(obj.dump());
}

// Remove song from queue by index.
void removeFromQueue(size_t index) {
	for (ssize_t i = queue.size()-1; i >= 0; i--) {
		if (queue[i].index == index) {
			queue.erase(queue.begin() + i);
			json obj;
			obj["remove_from_queue"] = index;
			broadcast(obj.dump());
			return;
		}
	}
}
