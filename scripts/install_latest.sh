#!/usr/bin/env bash
set -e

echo "Fetching latest release..."
URL=$(curl -s https://api.github.com/repos/AndyFilter/YeetMouse/releases/latest \
  | grep browser_download_url \
  | grep amd64.deb \
  | cut -d '"' -f 4)

echo "Downloading from $URL"
wget -q "$URL"

FILE=$(basename "$URL")
sudo dpkg -i "$FILE"
sudo apt-get install -f
