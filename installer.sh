#!/bin/bash

# Check that PWD contains no spaces.
case "$(pwd)" in
	*\ *)
		echo -e "\033[31mError: Current path contains spaces.\033[0m"
		echo -e "\033[32mEither move into a directory with no spaces or rename directories such that there are no spaces in '$(pwd)'\033[0m"
		exit 1
		;;
esac

echo "Running the installer for Ubuntu/Debian"
echo

# Install dependencies
echo -e "\033[32mInstalling dependencies...\033[0m"
sudo apt update
sudo apt install git build-essential libssl-dev lame libmp3lame-dev libpulse-dev libboost-all-dev yt-dlp
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mWarning: Installation of dependencies failed.\033[0m"
fi
echo
echo

# Check for git files
echo -e "\033[32mDownloading...\033[0m"
doesthecd=""
if [[ ! -f Makefile ]]; then
	git clone --depth 1 https://github.com/robotman2412/musicsys
	ec=$?
	if [[ $ec -ne 0 ]]; then
		echo
		echo -e "\033[31mError: Download failed.\033[0m"
		exit 1
	fi
	doesthecd="musicsys/"
	cd musicsys
fi

# Update git
git pull
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Download failed.\033[0m"
	exit 1
fi
git submodule update --init --recursive
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Download failed.\033[0m"
	exit 1
fi
echo
echo

# Build the program
echo -e "\033[32mBuilding...\033[0m"
systemctl --user stop musicsys.service
make
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Build failed.\033[0m"
	exit 1
fi
echo
echo

# Create systemd service
echo -e "\033[32mCreating service...\033[0m"

echo "[Unit]" > musicsys.service
echo "Description=Web-based music system" >> musicsys.service
echo "After=graphical.target" >> musicsys.service
echo "" >> musicsys.service
echo "[Service]" >> musicsys.service
echo "ExecStart=/usr/bin/bash -- $(echo "$(pwd)/service.sh" | sed -e 's/ /\\\\\\x20/g') $(echo "$(pwd)" | sed -e 's/ /\\\\\\x20/g')" >> musicsys.service
echo "Type=simple" >> musicsys.service
echo "" >> musicsys.service
echo "[Install]" >> musicsys.service
echo "WantedBy=default.target" >> musicsys.service
echo "" >> musicsys.service

mkdir -p ~/.config/systemd/user/
cp musicsys.service ~/.config/systemd/user/musicsys.service
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Service installation failed.\033[0m"
	exit 1
fi
systemctl --user daemon-reload
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Service installation failed.\033[0m"
	exit 1
fi
systemctl --user disable musicsys.service
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Service installation failed.\033[0m"
	exit 1
fi
systemctl --user enable musicsys.service
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Service installation failed.\033[0m"
	exit 1
fi
systemctl --user start musicsys.service
ec=$?
if [[ $ec -ne 0 ]]; then
	echo
	echo -e "\033[31mError: Service installation failed.\033[0m"
	exit 1
fi

echo
echo -e "\033[32mInstalled successfully!\033[0m"
echo "You must keep the downloaded files, they are required for the service to run."
echo "The service runs for your user, and will start when you log in."
echo "To uninstall later, run ${doesthecd}uninstall.sh"
echo
