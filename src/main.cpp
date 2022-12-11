
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
	
		/* The Sample format to use */
	// static const pa_sample_spec ss = {
	// 	.format = PA_SAMPLE_S16NE,
	// 	.rate = 44100,
	// 	.channels = 2
	// };
	
	// pa_simple *s = NULL;
	// int ret = 1;
	// int error;
	
	// // Create a decoder.
	// // lame_t lame = lame_init();
	// // lame_set_in_samplerate(lame, 44100);
	// // lame_set_VBR(lame, vbr_default);
	// // lame_init_params(lame);
	// hip_t hip = hip_decode_init();
	
	// FILE *mp3 = fopen("data/songs/1.mp3", "rb");
	// if (!mp3) return 1;
	
	// /* Create a new playback stream */
	// if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
	// 	fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
	// 	goto finish;
	// }
	
	// uint8_t raw[2048];
	// int16_t left[BUFSIZE*2];
	// int16_t right[BUFSIZE];
	// mp3data_struct mp3_type;
	// for (;;) {
	// 	ssize_t num_out = 0;
	// 	while (num_out == 0) {
	// 		ssize_t r = fread(raw, 1, sizeof(raw), mp3);
	// 		num_out = hip_decode_headers(hip, raw, r, left, right, &mp3_type);
	// 		if (num_out < 0) return 2;
	// 	}
		
	// 	// Re-interlace the audio data.
	// 	for (ssize_t i = BUFSIZE-1; i >= 0; i--) {
	// 		left[i*2+0] = left[i];
	// 		left[i*2+1] = right[i];
	// 	}
		
	// 	/* ... and play it */
	// 	if (pa_simple_write(s, left, num_out*4, &error) < 0) {
	// 		fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
	// 		goto finish;
	// 	}
	// }
	
	// /* Make sure that every single sample was played */
	// if (pa_simple_drain(s, &error) < 0) {
	// 	fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
	// 	goto finish;
	// }
	
	// ret = 0;
	
	// finish:
	
	// if (s)
	// 	pa_simple_free(s);
	
	stopHttpServer();
	stopWebsocketServer();
	return 0;
}

void handleMessage(std::string in) {
	std::cout << in << std::endl;
}
