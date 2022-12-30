#!/bin/bash

echo "Running the installer for Ubuntu/Debian"

# Install dependencies
sudo apt update
sudo apt install git build-essential libssl-dev lame libmp3lame-dev libpulse-dev libboost-all-dev yt-dlp

# Check for git files
if [[ ! -f Makefile ]]; then
	git clone --depth 1 https://github.com/robotman2412/musicsys
	cd musicsys
fi

# Update git
git pull
git submodule update --init --recursive

# Build the program
make
