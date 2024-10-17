#!/bin/bash

# Install the driver and activate the dkms module
sudo make setup_dkms
sudo dkms install -m leetmouse-driver -v 0.9.0 # Enter the version you determined from the Makefile earlier in here
