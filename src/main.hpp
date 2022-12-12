
#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <iostream>
#include <string>

// Load and play song by ID.
void playNow(uint id);
// Delete song by ID.
void deleteSong(uint id);
// Add song to queue by ID.
void addToQueue(uint id);
// Skips the current song.
void skipSong();
// Remove song from queue by index.
void removeFromQueue(size_t index);
// Broadcast song status.
void sendSongStatus();
