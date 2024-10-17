#!/bin/bash
# Uninstall the driver
sudo dkms remove -m leetmouse-driver -v 0.9.0
sudo make remove_dkms
sudo rmmod leetmouse
