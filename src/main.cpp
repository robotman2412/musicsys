
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
#include <sys/time.h>

#include <queue>
#include <async_queue.hpp>

#include <pulse/simple.h>
#include <pulse/error.h>

#include <lame/lame.h>

#define FFT_WIDTH 100
#define FFT_TYPE int32_t
#define USE_FFT_SCALE 1
#define GRAPH_STYLES 3
int graphStyle = 0;
int graphColorIndex = 0;

static uint32_t graphColors[] = {
	0xff0000,
	0xffff00,
	0x00ff00,
	0x0000ff,
};
static const size_t numGraphColors = sizeof(graphColors) / sizeof(uint32_t);

MpegPlayer player;
Song nowPlaying;

std::vector<Song> queue;
uint64_t queue_index = 0;

std::map<uint, Song> songs;
std::vector<Download *> downloads;
uint new_id_index = 0;

FFTSpectrum<FFT_TYPE> spectrum;
ASQueue<FFTData<FFT_TYPE>> analysed;
FFTData<FFT_TYPE> fftCur;
FFTData<FFT_TYPE> fftLast;
float fftRate = 60.0;

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
	
	// Create the testing FFT..
	player.sampleCallback = [](double songTime, size_t sampleCount, int16_t *left, int16_t *right, float sampleRate) -> void {
		// Create a temporary buffer.
		std::vector<FFT_TYPE> sampleBuf;
		sampleBuf.resize(sampleCount);
		
		// Mix samples into a buffer.
		for (size_t i = 0; i < sampleCount; i++) {
			sampleBuf[i] = (FFT_TYPE) left[i] + (FFT_TYPE) right[i];
		}
		
		// Send samples through FFT.
		size_t i = 0;
		for (FFTData<FFT_TYPE> &entry: spectrum.feedSamples(songTime, sampleBuf)) {
			analysed.send(entry);
			i++;
		}
	};
	
	// Default volume.
	player.setVolume(0.4);
	
	// Send metadata from time to time.
	uint64_t nextTime = micros();
	while (1) {
		sendSongStatus();
		if (player.getStatus() == MpegPlayer::FINISHED) {
			player.acknowledge();
			skipSong();
			nextTime += (uint64_t) ((double) 1000000 / fftRate);
			
		} else if (player.getStatus() == MpegPlayer::PLAYING) {
			if (fftCur.coeff.size()) {
				// Am send FFT data?
				sendFFTStatus();
			}
			
			if (analysed.tryRecv(fftCur)) {
				// Use timestamp from FFT datas.
				double dt = fftCur.songTime - player.tell();
				nextTime  = micros() + dt * 1000000;
				
			} else {
				// Invalidate FFT datas.
				fftCur.coeff.clear();
				nextTime += 100000;
			}
			
		} else {
			nextTime = micros() + 100000;
		}
		
		int64_t tdel = nextTime - micros();
		if (tdel > 0) {
			usleep(tdel);
		} else {
			nextTime = micros();
		}
	}
	
	// Close servers.
	stopHttpServer();
	stopWebsocketServer();
	return 0;
}

// Get time in microseconds.
int64_t micros() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t) tv.tv_sec * 1000000ULL + tv.tv_usec;
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
		
	} else if (data["set_play_current_time"].is_number()) {
		// Seek command.
		player.seek(data["set_play_current_time"]);
		analysed.clear();
		
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

// Broadcast FFT data.
void sendFFTStatus() {
	// Used for BEAT DETECT.
	static double prevAvg    = 0;
	static double curAvg     = 0;
	static double prevDelta  = 0;
	static double curDelta   = 0;
	static double beatThresh = 3;
	static bool   beat       = false;
	
	// Create an array to pack data into.
	json obj;
	json arr = json::array();
	
	// Make an average of the FFT data.
	prevAvg = curAvg;
	curAvg  = 0;
	for (size_t i = 0; i < 10; i++) {
		curAvg += abs(fftCur.coeff[i]);
	}
	curAvg /= fftCur.coeff.size();
	
	// Make acceleration measurements.
	prevDelta = curDelta;
	curDelta  = curAvg / prevAvg;
	
	// Determine COEFF.
	if (curDelta - prevDelta >= beatThresh) {
		if (!beat) {
			graphColorIndex = (graphColorIndex + 1) % numGraphColors;
			obj["fft_color"] = graphColors[graphColorIndex];
			beat = true;
		}
	} else {
		beat = false;
	}
	
	// Prepare FFT data.
#if USE_FFT_SCALE
	// Determine scaling parameters.
	double min = 0.6/30;
	double max = 1.5/30;
	double inc = (max - min) / (fftCur.coeff.size() - 1);
	double cur = min;
	
	// Scale datas.
	for (size_t i = 0; i < fftCur.coeff.size(); i++, cur += inc) {
		arr.push_back(abs(fftCur.coeff[i] * cur));
	}
#else
	for (auto i: analisys) arr.push_back(abs(i/30.0));
#endif
	
	// Send message.
	obj["fft_data"] = arr;
	broadcast(obj.dump());
	
	fftLast = fftCur;
}

// Load and play song by ID.
void playNow(uint id) {
	if (!songs[id].valid) return;
	
	std::string path = "data/songs/" + std::to_string(id) + ".mp3";
	player.loadFile(path);
	player.start();
	if (player.getStatus() == MpegPlayer::PLAYING) {
		// Clear FFT queue so the FFT doesn't break.
		analysed.clear();
		
		// Switch up graph style.
		graphStyle = (graphStyle + 1) % GRAPH_STYLES;
		
		// Set now playing.
		nowPlaying = songs[id];
		nowPlaying.currentTime = 0;
		
		// Broadcast now playing.
		json msg;
		msg["song_now_playing"] = nowPlaying.toJson();
		msg["playing"] = true;
		msg["graph_style"] = graphStyle;
		broadcast(msg.dump());
		std::cout << "Now playing: " << songs[id].name << std::endl;
	}
}

// Skips the current song.
void skipSong() {
	// Stop player.
	player.pause();
	
	// Clear FFT queue.
	analysed.clear();
	
	if (queue.size()) {
		// Load more from queue
		Song next = queue[0];
		removeFromQueue(next.index);
		playNow(next.id);
	} else {
		// Stop playing.
		player.clear();
		nowPlaying = Song();
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
