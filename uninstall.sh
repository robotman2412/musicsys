#!/bin/bash

# Stop service.
echo -e "\033[32mStopping service...\033[0m"
sudo systemctl --machine=$USER@.host --user stop musicsys.service
sudo systemctl --machine=$USER@.host --user disable musicsys.service

# Remove from systemd.
echo -e "\033[32mRemoving from systemd...\033[0m"
sudo rm /etc/systemd/user/musicsys.service
sudo systemctl --machine=$USER@.host --user daemon-reload
echo

echo "Service stopped, disabled, systemd service removed."
echo "You can now safely delete the musicsys files."
