#!/bin/bash

echo "Running the installer for Ubuntu/Debian"

# Install dependencies
echo -e "\033[32mInstalling dependencies...\033[0m"
sudo apt update
sudo apt install git build-essential libssl-dev lame libmp3lame-dev libpulse-dev libboost-all-dev yt-dlp
echo
echo

# Check for git files
echo -e "\033[32mDownloading...\033[0m"
if [[ ! -f Makefile ]]; then
	git clone --depth 1 https://github.com/robotman2412/musicsys
	cd musicsys
fi

# Update git
git pull
git submodule update --init --recursive
echo
echo

# Build the program
echo -e "\033[32mBuilding...\033[0m"
make
echo
echo
