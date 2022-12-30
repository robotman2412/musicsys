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

# Create systemd service
echo -e "\033[32mCreating service...\033[0m"

echo "[Unit]" > musicsys.service
echo "Description=Web-based music system" >> musicsys.service
echo "After=graphical.target" >> musicsys.service
echo "" >> musicsys.service
echo "[Service]" >> musicsys.service
echo "ExecStart=/usr/bin/bash -c -- '$(echo "$(pwd)/service.sh" | sed -e 's/ /\\x20/g')' '$(echo "$(pwd)" | sed -e 's/ /\\\\x20/g')'" >> musicsys.service
echo "Type=simple" >> musicsys.service
echo "User=$USER" >> musicsys.service
echo "Group=$USER" >> musicsys.service
echo "" >> musicsys.service
echo "[Install]" >> musicsys.service
echo "WantedBy=graphical.target" >> musicsys.service
echo "" >> musicsys.service

echo
echo
