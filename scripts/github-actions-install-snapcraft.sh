#!/bin/bash
sudo chown root:root /
sudo apt-get remove -qy lxd lxd-client
sudo snap install lxd
sudo lxd init --auto
sudo snap install snapcraft --classic
