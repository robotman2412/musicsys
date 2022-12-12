
#include "main.hpp"
#include "httpserver.hpp"
#include "wsserver.hpp"
#include "player.hpp"

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

int main(int argc, char **argv) {
	startHttpServer(8080, "./web/", 1);
	startWebsocketServer(6969, 1);
	MpegPlayer player;
	player.loadFile("data/songs/1.mp3");
	player.setVolume(0.4);
	player.start();
	
	while(1);
	
	stopHttpServer();
	stopWebsocketServer();
	return 0;
}

void handleMessage(std::string in) {
	std::cout << in << std::endl;
}
