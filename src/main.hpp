
#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <iostream>
#include <string>

// Load and play song by ID.
void playNow(uint id);
// Broadcast song status.
void sendSongStatus();
