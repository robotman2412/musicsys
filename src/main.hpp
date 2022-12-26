
#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <iostream>
#include <string>

// Get time in microseconds.
int64_t micros();
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
// Broadcast FFT data.
void sendFFTStatus();
// Handle all upload thing handler devices.
void handleDownloads();
// Control-C handler device.
extern "C" void onInterrupt(int signum);
