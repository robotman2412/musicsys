
#pragma once

#include <string>

extern void handleMessage(std::string in);
void broadcast(std::string in);
void startWebsocketServer(int port, int threads);
void stopWebsocketServer();
