
#include "main.hpp"
#include "httpserver.hpp"
#include "wsserver.hpp"
#include "player.hpp"
#include "data.hpp"

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

int main(int argc, char **argv) {
	startHttpServer(8080, "./web/", 1);
	startWebsocketServer(6969, 1);
	
	usleep(2000000);
	playNow(2);
	
	while (1) {
		sendSongStatus();
		if (player.getStatus() == MpegPlayer::PLAYING) {
			usleep(200000);
		} else {
			usleep(1000000);
		}
	}
	
	stopHttpServer();
	stopWebsocketServer();
	return 0;
}

void handleMessage(Messager socket, std::string in) {
	std::cout << in << std::endl;
	
	json data = json::parse(in);
	if (data.contains("set_playing") && data["set_playing"].is_boolean()) {
		// Play/pause command.
		if (data["set_playing"]) player.start();
		else player.pause();
		
		// Broadcast playing status.
		if (player.getStatus() == MpegPlayer::PLAYING) {
			broadcast("{\"playing\":true}");
		} else {
			broadcast("{\"playing\":false}");
		}
		
	} else if (data.contains("play_song_now") && data["play_song_now"].is_number()) {
		// Play song command.
		playNow(data["play_song_now"]);
		
	} else if (data.contains("set_volume") && data["set_volume"].is_number()) {
		// Set volume command.
		player.setVolume(data["set_volume"]);
		
	}
}

// Broadcast song status.
void sendSongStatus() {
	nowPlaying.currentTime = player.tell();
	nowPlaying.duration    = player.duration();
	json msg;
	msg["song_now_playing"] = nowPlaying.toJson();
	msg["playing"] = player.getStatus() == MpegPlayer::PLAYING;
	msg["set_volume"] = player.getVolume();
	broadcast(msg.dump());
}

// Load and play song by ID.
void playNow(uint id) {
	Song next = Song::load(id);
	if (!next.valid) return;
	
	std::string path = "data/songs/" + std::to_string(id) + ".mp3";
	player.loadFile(path);
	player.start();
	if (player.getStatus() == MpegPlayer::PLAYING) {
		// This indicates successful loading.
		nowPlaying = next;
		nowPlaying.currentTime = 0;
		json msg;
		msg["song_now_playing"] = nowPlaying.toJson();
		msg["playing"] = true;
		broadcast(msg.dump());
	}
}
