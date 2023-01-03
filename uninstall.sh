#!/bin/bash

# Stop service.
echo -e "\033[32mStopping service...\033[0m"
systemctl --user stop musicsys.service
systemctl --user disable musicsys.service

# Remove from systemd.
echo -e "\033[32mRemoving from systemd...\033[0m"
rm -f ~/.config/systemd/user/musicsys.service
systemctl --user daemon-reload
echo

echo "Service stopped, disabled, systemd service removed."
echo "You can now safely delete the musicsys files."
