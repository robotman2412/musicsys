
#pragma once

#include <string>
#include <functional>

typedef std::function<void(std::string)> Messager;

extern void handleAccepted(Messager socket);
extern void handleMessage(Messager socket, std::string in);
void broadcast(std::string in);
void startWebsocketServer(int port, int threads);
void stopWebsocketServer();
