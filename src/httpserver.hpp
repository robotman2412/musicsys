
#pragma once

#include <string>

void startHttpServer(int port, std::string doc_root, int threads);
void stopHttpServer();
